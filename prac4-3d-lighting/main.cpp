/*
 * main.cpp  –  COS344 Practical 4: Translucency, Textures and Light
 * ===================================================================
 * Student: Shavir Vallabh  u23718146
 *
 * SCENE: Semi-translucent 3D glass golf ball on a flat plane.
 *        Point light inside the ball shines through the glass onto the floor.
 *
 * CONTROLS:
 *   Scene rotation:   W/S=X   A/D=Y   Q/E=Z
 *   Light position:   Arrows=X/Y   </>=Z
 *   Sphere segments:  Z=more  X=fewer
 *   Plane divisions:  C=more  V=fewer
 *   Textures:         B=colour  N=displacement  M=alpha
 *   Floor colour:     [=prev  ]=next
 *   Ball colour:      ;=prev  '=next
 *   Light colour:     ,=prev  .=next
 *   Alpha value:      +/-
 *   Reset:            Space
 *   Wireframe:        Enter
 *   Quit:             Escape
 *
 * ARCHITECTURE:
 *   - mat4.hpp: custom 4x4 matrix math (no GLM)
 *   - TextureGen.hpp: generates PPM textures procedurally
 *   - Geometry.hpp: Sphere and Plane classes with runtime rebuild
 *   - vertex4.glsl / fragment4.glsl: full pipeline
 *   - Single shader program with mode uniforms (isFloor, useColourTex, etc.)
 *   - worldMatrix accumulates all WASD rotations (never resets) 
 *   - Light position stored in LOCAL space, transformed to world space each frame
 *   - Draw order: floor first (opaque), then ball (translucent with GL_BLEND)
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "shader.hpp"
#include "Mat4.hpp"
#include "TextureGen.hpp"
#include "Geometry.hpp"

using namespace std;

// ============================================================
// Window
// ============================================================
const int WIN_W = 1000;
const int WIN_H = 1000;

// ============================================================
// INITIAL / DEFAULT VALUES (used for Space reset)
// ============================================================
const int   INIT_LAT_SEGS  = 20;
const int   INIT_LON_SEGS  = 20;
const int   INIT_PLANE_DIV = 16;
const float INIT_ALPHA     = 0.55f;
const float BALL_RADIUS    = 1.0f;
const float BALL_Y         = BALL_RADIUS;  // ball rests on plane (y=radius)
const float PLANE_Y        = 0.0f;

// ============================================================
// Colour palettes (10 each for floor and ball, 9 for light)
// ============================================================
struct Col3 { float r,g,b; };
struct Col4 { float r,g,b,a; };

const Col3 FLOOR_COLOURS[10] = {
    {1.0f,0.0f,0.0f},  // red
    {0.0f,1.0f,0.0f},  // green
    {0.0f,0.0f,1.0f},  // blue
    {1.0f,1.0f,1.0f},  // white
    {0.0f,0.0f,0.0f},  // black
    {0.8f,0.5f,0.1f},  // amber
    {0.5f,0.0f,0.5f},  // purple
    {0.0f,0.5f,0.5f},  // teal
    {0.9f,0.9f,0.2f},  // yellow
    {0.4f,0.2f,0.1f},  // brown
};

const Col3 BALL_COLOURS[10] = {
    {1.0f,0.0f,0.0f},  // red
    {0.0f,1.0f,0.0f},  // green
    {0.0f,0.0f,1.0f},  // blue
    {1.0f,1.0f,1.0f},  // white (default glass look)
    {0.0f,0.0f,0.0f},  // black
    {0.8f,0.8f,0.8f},  // light grey (default)
    {0.0f,0.8f,0.8f},  // cyan
    {0.8f,0.0f,0.8f},  // magenta
    {1.0f,0.8f,0.2f},  // gold
    {0.3f,0.8f,0.3f},  // mint green
};

const Col3 LIGHT_COLOURS[9] = {
    {1.0f,1.0f,1.0f},  // white (default)
    {1.0f,0.0f,0.0f},  // red
    {0.0f,1.0f,0.0f},  // green
    {0.0f,0.0f,1.0f},  // blue
    {1.0f,1.0f,0.0f},  // yellow
    {1.0f,0.5f,0.0f},  // orange
    {0.5f,0.0f,1.0f},  // violet
    {0.0f,1.0f,1.0f},  // cyan
    {1.0f,0.0f,0.5f},  // rose
};

// ============================================================
// Global state
// ============================================================
bool wireframe  = false;
double lastEnter = 0.0;

// World rotation matrix (accumulated, never reset on key press)
Mat4 worldMatrix;

// Light position in LOCAL space (relative to ball centre = 0,0,0 local)
Vec3 lightLocalPos = {0.0f, 0.0f, 0.0f};  // starts at ball centre

// Runtime-adjustable state
int   latSegs      = INIT_LAT_SEGS;
int   lonSegs      = INIT_LON_SEGS;
int   planeDiv     = INIT_PLANE_DIV;
float alphaValue   = INIT_ALPHA;
int   floorColIdx  = 3;   // white
int   ballColIdx   = 5;   // light grey
int   lightColIdx  = 0;   // white

bool  useColourTex = false;
bool  useDispTex   = false;
bool  useAlphaTex  = false;

// Geometry objects
Sphere sphere;
Plane  plane;

// Texture IDs
GLuint texColour = 0, texDisp = 0, texAlpha = 0;

// Shader uniform locations
GLint uModel, uView, uProjection, uNormalMatrix;
GLint uIsFloor, uWireframe;
GLint uUseColourTex, uUseAlphaTex;
GLint uUseDisplacement, uDispStrength, uDispTexture;
GLint uColourTexture, uAlphaTexture;
GLint uBallColour, uAlphaValue;
GLint uFloorColour;
GLint uLightPos, uLightColour, uCameraPos;

// ============================================================
// GLFW helpers
// ============================================================
GLFWwindow* createWindow() {
    glewExperimental = true;
    if (!glfwInit()) { cerr << "GLFW init failed\n"; return nullptr; }
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    GLFWwindow* w = glfwCreateWindow(WIN_W,WIN_H,"u23718146",nullptr,nullptr);
    if (!w) { cerr<<"Window failed\n"; glfwTerminate(); return nullptr; }
    glfwMakeContextCurrent(w);
    glewExperimental = true;
    if (glewInit()!=GLEW_OK) { cerr<<"GLEW failed\n"; return nullptr; }
    return w;
}

// ============================================================
// Key callback
// ============================================================
const float ROT_STEP   = 0.04f;
const float LIGHT_STEP = 0.08f;
const float ALPHA_STEP = 0.05f;

void keyCallback(GLFWwindow* win, int key, int, int action, int) {
    if (action == GLFW_RELEASE) return;

    switch (key) {
    // ---- Scene rotation (world matrix accumulates) ----
    case GLFW_KEY_W: worldMatrix = mat4::rotateX(-ROT_STEP)*worldMatrix; break;
    case GLFW_KEY_S: worldMatrix = mat4::rotateX( ROT_STEP)*worldMatrix; break;
    case GLFW_KEY_A: worldMatrix = mat4::rotateY(-ROT_STEP)*worldMatrix; break;
    case GLFW_KEY_D: worldMatrix = mat4::rotateY( ROT_STEP)*worldMatrix; break;
    case GLFW_KEY_E: worldMatrix = mat4::rotateZ(-ROT_STEP)*worldMatrix; break;
    case GLFW_KEY_Q: worldMatrix = mat4::rotateZ( ROT_STEP)*worldMatrix; break;

    // ---- Light local position (arrow keys + < >) ----
    // "Local" means relative to ball centre in the pre-worldMatrix frame
    case GLFW_KEY_UP:    lightLocalPos.y += LIGHT_STEP; break;
    case GLFW_KEY_DOWN:  lightLocalPos.y -= LIGHT_STEP; break;
    case GLFW_KEY_LEFT:  lightLocalPos.x -= LIGHT_STEP; break;
    case GLFW_KEY_RIGHT: lightLocalPos.x += LIGHT_STEP; break;
    case GLFW_KEY_PERIOD:  // '>'
        if (action==GLFW_PRESS || action==GLFW_REPEAT) lightLocalPos.z += LIGHT_STEP; break;
    case GLFW_KEY_COMMA:   // '<'
        if (action==GLFW_PRESS || action==GLFW_REPEAT) lightLocalPos.z -= LIGHT_STEP; break;

    // ---- Sphere vertex count ----
    case GLFW_KEY_Z:
        latSegs = min(latSegs+2, 64); lonSegs = min(lonSegs+2, 64);
        sphere.latSegs=latSegs; sphere.lonSegs=lonSegs;
        sphere.rebuild(useDispTex);
        break;
    case GLFW_KEY_X:
        latSegs = max(latSegs-2, 4); lonSegs = max(lonSegs-2, 4);
        sphere.latSegs=latSegs; sphere.lonSegs=lonSegs;
        sphere.rebuild(useDispTex);
        break;

    // ---- Plane subdivision count ----
    case GLFW_KEY_C:
        planeDiv = min(planeDiv+4, 64);
        plane.divs=planeDiv; plane.rebuild();
        break;
    case GLFW_KEY_V:
        planeDiv = max(planeDiv-4, 2);
        plane.divs=planeDiv; plane.rebuild();
        break;

    // ---- Texture toggles ----
    case GLFW_KEY_B:
        if (action==GLFW_PRESS) useColourTex = !useColourTex;
        break;
    case GLFW_KEY_N:
        if (action==GLFW_PRESS) {
            useDispTex = !useDispTex;
            sphere.rebuild(useDispTex);  // actual geometry changes
        }
        break;
    case GLFW_KEY_M:
        if (action==GLFW_PRESS) useAlphaTex = !useAlphaTex;
        break;

    // ---- Floor colour cycle ---- ([ and ])
    case GLFW_KEY_LEFT_BRACKET:
        if (action==GLFW_PRESS) floorColIdx=(floorColIdx+9)%10; break;
    case GLFW_KEY_RIGHT_BRACKET:
        if (action==GLFW_PRESS) floorColIdx=(floorColIdx+1)%10; break;

    // ---- Ball colour cycle ---- (; and ')
    case GLFW_KEY_SEMICOLON:
        if (action==GLFW_PRESS) ballColIdx=(ballColIdx+9)%10; break;
    case GLFW_KEY_APOSTROPHE:
        if (action==GLFW_PRESS) ballColIdx=(ballColIdx+1)%10; break;

    // ---- Light colour cycle ---- (, and .)
    // Note: PERIOD/COMMA are also < and > — but < > need Shift on many keyboards
    // so we use GLFW_KEY_PERIOD (without shift = '.') for light too; resolve with mods
    // We handle this by using different keys to avoid conflict:
    // Actually spec says < and > for light Z movement. On standard keyboard:
    //   < = Shift+, and > = Shift+.
    // So plain , and . = light colour cycle, Shift+, and Shift+. = light Z movement
    // We detect this via mods in a separate approach. For simplicity:
    //   , (without shift) = prev light colour
    //   . (without shift) = next light colour
    // These are handled here; the light Z movement above is handled by PERIOD/COMMA too
    // but action filter and the fact they are the same key means we need mods.
    // Simpler: use F1/F2 for light colour to avoid confusion. But spec doesn't mandate keys.
    // We'll use F1=prev, F2=next for light colour (clear and unambiguous).
    case GLFW_KEY_F1:
        if (action==GLFW_PRESS) lightColIdx=(lightColIdx+8)%9; break;
    case GLFW_KEY_F2:
        if (action==GLFW_PRESS) lightColIdx=(lightColIdx+1)%9; break;

    // ---- Alpha value ---- (+ and -)
    case GLFW_KEY_EQUAL:  // = is + without shift on most keyboards
    case GLFW_KEY_KP_ADD:
        alphaValue = min(1.0f, alphaValue+ALPHA_STEP); break;
    case GLFW_KEY_MINUS:
    case GLFW_KEY_KP_SUBTRACT:
        alphaValue = max(0.05f, alphaValue-ALPHA_STEP); break;

    // ---- Wireframe toggle ----
    case GLFW_KEY_ENTER:
        if (action==GLFW_PRESS) {
            double now=glfwGetTime();
            if (now-lastEnter>0.25){wireframe=!wireframe;lastEnter=now;}
        }
        break;

    // ---- RESET ----
    case GLFW_KEY_SPACE:
        if (action==GLFW_PRESS) {
            worldMatrix    = mat4::identity();
            lightLocalPos  = {0.0f,0.0f,0.0f};
            latSegs = lonSegs = INIT_LAT_SEGS;
            planeDiv       = INIT_PLANE_DIV;
            alphaValue     = INIT_ALPHA;
            floorColIdx    = 3;
            ballColIdx     = 5;
            lightColIdx    = 0;
            useColourTex   = false;
            useDispTex     = false;
            useAlphaTex    = false;
            wireframe      = false;
            sphere.latSegs = latSegs;
            sphere.lonSegs = lonSegs;
            sphere.rebuild(false);
            plane.divs     = planeDiv;
            plane.rebuild();
            cout<<"Scene reset to initial state.\n";
        }
        break;

    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(win,GLFW_TRUE); break;

    default: break;
    }
}

// ============================================================
// Upload uniforms for a given mode
// ============================================================
void setUniforms(bool isFloor, Mat4& model, Mat4& view, Mat4& proj,
                 Vec3& lightWorldPos, Vec3& camPos)
{
    // MVP
    glUniformMatrix4fv(uModel,      1, GL_FALSE, model.ptr());
    glUniformMatrix4fv(uView,       1, GL_FALSE, view.ptr());
    glUniformMatrix4fv(uProjection, 1, GL_FALSE, proj.ptr());

    // Normal matrix = upper-left 3x3 of transpose(inverse(model))
    // For our case model is only rotation+translation (no non-uniform scale),
    // so normalMatrix = upper-left 3x3 of model (rotation part).
    // We compute it directly.
    float nm[9] = {
        model.m[0][0], model.m[0][1], model.m[0][2],
        model.m[1][0], model.m[1][1], model.m[1][2],
        model.m[2][0], model.m[2][1], model.m[2][2]
    };
    glUniformMatrix3fv(uNormalMatrix, 1, GL_FALSE, nm);

    // Mode
    glUniform1i(uIsFloor,   isFloor ? 1 : 0);
    glUniform1i(uWireframe, wireframe ? 1 : 0);

    // Textures
    glUniform1i(uUseColourTex,    useColourTex ? 1 : 0);
    glUniform1i(uUseAlphaTex,     useAlphaTex  ? 1 : 0);
    glUniform1i(uUseDisplacement, useDispTex   ? 1 : 0);
    glUniform1f(uDispStrength, 0.12f);

    // Bind textures to units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texColour);
    glUniform1i(uColourTexture, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texDisp);
    glUniform1i(uDispTexture, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texAlpha);
    glUniform1i(uAlphaTexture, 2);

    // Colours
    Col3 bc = BALL_COLOURS[ballColIdx];
    glUniform4f(uBallColour, bc.r, bc.g, bc.b, alphaValue);
    glUniform1f(uAlphaValue, alphaValue);

    Col3 fc = FLOOR_COLOURS[floorColIdx];
    glUniform3f(uFloorColour, fc.r, fc.g, fc.b);

    Col3 lc = LIGHT_COLOURS[lightColIdx];
    glUniform3f(uLightColour, lc.r, lc.g, lc.b);

    // Light world position: ball is at (0, BALL_Y, 0) in object space,
    // then worldMatrix is applied. Light local pos is relative to ball centre.
    // World light pos = worldMatrix * (ballCentre + lightLocalPos)
    Vec4 localLight = {
        lightLocalPos.x,
        BALL_Y + lightLocalPos.y,
        lightLocalPos.z,
        1.0f
    };
    Vec4 worldLight = worldMatrix * localLight;
    glUniform3f(uLightPos, worldLight.x, worldLight.y, worldLight.z);

    glUniform3f(uCameraPos, camPos.x, camPos.y, camPos.z);
}

// ============================================================
// main
// ============================================================
int main() {
    GLFWwindow* window = createWindow();
    if (!window) return -1;

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetKeyCallback(window, keyCallback);

    // Generate textures (writes PPM files and loads them as GL textures)
    TextureGen::generateAll();
    texColour = TextureGen::loadPPM("colour_texture.ppm");
    texDisp   = TextureGen::loadPPM("displacement_texture.ppm");
    texAlpha  = TextureGen::loadPPM("alpha_texture.ppm");

    if (!texColour || !texDisp || !texAlpha) {
        cerr << "Texture load failed\n";
        glfwTerminate(); return -1;
    }

    // Load shaders
    GLuint prog = LoadShaders("vertex3d.glsl", "fragment3d.glsl");
    if (!prog) { cerr << "Shader load failed\n"; glfwTerminate(); return -1; }
    glUseProgram(prog);

    // Cache uniform locations
    uModel          = glGetUniformLocation(prog, "model");
    uView           = glGetUniformLocation(prog, "view");
    uProjection     = glGetUniformLocation(prog, "projection");
    uNormalMatrix   = glGetUniformLocation(prog, "normalMatrix");
    uIsFloor        = glGetUniformLocation(prog, "isFloor");
    uWireframe      = glGetUniformLocation(prog, "wireframe");
    uUseColourTex   = glGetUniformLocation(prog, "useColourTex");
    uUseAlphaTex    = glGetUniformLocation(prog, "useAlphaTex");
    uUseDisplacement= glGetUniformLocation(prog, "useDisplacement");
    uDispStrength   = glGetUniformLocation(prog, "dispStrength");
    uColourTexture  = glGetUniformLocation(prog, "colourTexture");
    uDispTexture    = glGetUniformLocation(prog, "dispTexture");
    uAlphaTexture   = glGetUniformLocation(prog, "alphaTexture");
    uBallColour     = glGetUniformLocation(prog, "ballColour");
    uAlphaValue     = glGetUniformLocation(prog, "alphaValue");
    uFloorColour    = glGetUniformLocation(prog, "floorColour");
    uLightPos       = glGetUniformLocation(prog, "lightPos");
    uLightColour    = glGetUniformLocation(prog, "lightColour");
    uCameraPos      = glGetUniformLocation(prog, "cameraPos");

    // Build initial geometry
    sphere.radius  = BALL_RADIUS;
    sphere.latSegs = latSegs;
    sphere.lonSegs = lonSegs;
    sphere.init();
    sphere.rebuild(false);

    plane.size   = 10.0f;
    plane.divs   = planeDiv;
    plane.uvTile = 4.0f;
    plane.init();
    plane.rebuild();

    // Camera (fixed, looking at scene from above-and-front)
    Vec3 camPos    = {0.0f, 4.0f, 8.0f};
    Vec3 camTarget = {0.0f, BALL_Y, 0.0f};
    Vec3 camUp     = {0.0f, 1.0f, 0.0f};
    Mat4 view      = mat4::lookAt(camPos, camTarget, camUp);
    Mat4 proj      = mat4::perspective((float)M_PI/4.0f,
                                        (float)WIN_W/WIN_H, 0.1f, 100.0f);

    // OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.10f, 0.10f, 0.15f, 1.0f);  // dark blue-grey background

    cout << "COS344 P4 - Glass Golf Ball\n"
         << "  W/S/A/D/Q/E: rotate scene\n"
         << "  Arrows: move light X/Y   < >: move light Z\n"
         << "  Z/X: sphere segments   C/V: plane divisions\n"
         << "  B: colour texture   N: displacement   M: alpha texture\n"
         << "  [/]: floor colour   ;/': ball colour   F1/F2: light colour\n"
         << "  +/-: alpha value   Space: reset   Enter: wireframe   Esc: quit\n";

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog);

        /*
         * DRAW ORDER (painter's algorithm for transparency):
         * 1. Floor (opaque) — drawn first with depth write enabled
         * 2. Ball  (translucent) — drawn after with depth test still on
         *    so that the floor visible through the ball is correctly shown
         *
         * GL_BLEND is always on; floor fragment alpha = 1.0 so it's opaque.
         */
        Vec3 nullVec = {0,0,0};
        // ---- 1. Draw floor ----
        {
            // Plane model: at y=0, scaled to fill view, world rotation applied
            Mat4 planeModel = worldMatrix * mat4::translate(0, PLANE_Y, 0);

            setUniforms(true, planeModel, view, proj,
                        nullVec/*unused here, computed inside*/, camPos);
            // Recompute light world pos for the uniform:
            Vec4 lp = worldMatrix * Vec4(lightLocalPos.x,
                                         BALL_Y+lightLocalPos.y,
                                         lightLocalPos.z, 1.0f);
            glUniform3f(uLightPos, lp.x, lp.y, lp.z);

            plane.draw(wireframe);
        }

        // ---- 2. Draw ball (translucent) ----
        {
            // Ball model: translate up by radius so it rests on y=0 plane
            Mat4 ballModel = worldMatrix * mat4::translate(0, BALL_Y, 0);

            setUniforms(false, ballModel, view, proj,
                        nullVec/*unused for ball*/, camPos);

            // For correct translucency we ideally sort back-faces first.
            // Simple approach: draw back faces then front faces.
            glEnable(GL_CULL_FACE);

            // Pass 1: back faces (seen through the glass)
            glCullFace(GL_FRONT);
            sphere.draw(wireframe);

            // Pass 2: front faces (glass surface facing camera)
            glCullFace(GL_BACK);
            sphere.draw(wireframe);

            glDisable(GL_CULL_FACE);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteTextures(1,&texColour);
    glDeleteTextures(1,&texDisp);
    glDeleteTextures(1,&texAlpha);
    glDeleteProgram(prog);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
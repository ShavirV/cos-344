#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <random>
#include <chrono> 

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "shader.hpp"
#include "Mat4.hpp"
#include "Mesh.hpp"
#include "Shapes3d.hpp"



using namespace glm;
using namespace std;

const int WIN_W = 1000;
const int WIN_H = 1000;

bool   wireframe  = false;
double lastEnter  = 0.0;

//accumulates all wasd and ijkl transformations
Mat4 worldMatrix;

float bladeSpeed = 0.0f;   //radians/second, clamped ≥ 0
float bladeAngle = 0.0f;   //accumulated angle
double lastTime  = 0.0;

struct SceneObject {
    Mesh* mesh;
    Mat4  local;       // local-to-world placement (before worldMatrix)
    bool  isBlade;     // receives blade spin on top of local
};

vector<SceneObject> scene;

// Blade axis in the windmill's local space.
// The blades rotate around Z (they face toward +Z, spin in XY plane).
// This is the axis in LOCAL blade space; we pass it through worldMatrix
// for the actual rotation so it stays correct after world rotations.
const Vec3 BLADE_AXIS = {0.0f, 0.0f, 1.0f};

// Position of the axle in world space (before worldMatrix)
// Blades spin around this point.
const Vec3 BLADE_PIVOT = {0.0f, 5.8f, 0.52f};

// ---------------------------------------------------------------------------
// Colours matching De Zwaan reference photo
// ---------------------------------------------------------------------------
const RGB3 cWhite      = {0.95f, 0.95f, 0.93f}; // tower cladding
const RGB3 cDarkGreen  = {0.10f, 0.20f, 0.12f}; // dark trim / cap
const RGB3 cBladeFrame = {0.08f, 0.08f, 0.08f}; // black blade frame
const RGB3 cBladeSail  = {0.92f, 0.90f, 0.82f}; // cream sail panel
const RGB3 cMetal      = {0.55f, 0.60f, 0.62f}; // metallic axle
const RGB3 cWood       = {0.60f, 0.42f, 0.22f}; // wooden base/struts
const RGB3 cGrass      = {0.18f, 0.58f, 0.18f}; // course surface
const RGB3 cWall       = {0.45f, 0.35f, 0.28f}; // boundary walls (distinct)
const RGB3 cStartMat   = {0.85f, 0.70f, 0.15f}; // yellow starting mat
const RGB3 cHole       = {0.10f, 0.10f, 0.10f}; // dark golf hole
const RGB3 cBall       = {0.98f, 0.98f, 0.98f}; // white golf ball
const RGB3 cFlowerPink = {0.98f, 0.45f, 0.65f}; // pink flower heads
const RGB3 cStem       = {0.15f, 0.55f, 0.15f}; // flower stem green
const RGB3 cTreeTrunk  = {0.40f, 0.28f, 0.12f}; // tree trunk brown
const RGB3 cTreeTop    = {0.08f, 0.38f, 0.08f}; // dark tree green

// ---------------------------------------------------------------------------
// Helper: add a scene object
// ---------------------------------------------------------------------------
static void add(Mesh* m, Mat4 local, bool blade=false) {
    scene.push_back({m, local, blade});
}

// ---------------------------------------------------------------------------
// Course construction
// ---------------------------------------------------------------------------
/*
 * Course layout (XZ plane, Y=0 is ground level):
 *
 *   Z
 *   ^
 *   |  [tree] [flower]   [WINDMILL]   [flower] [tree]
 *   |
 *   |  +--wall--+--wall--+  tunnel  +--wall--+--wall--+
 *   |  |                                               |
 *   |  |   [ball]                        [hole]        |
 *   |  |                                               |
 *   |  +--wall-----------+----------wall--------------+
 *   |         [start mat here]
 *   +-----------------------------------------------------------> X
 *
 * Course centre at (0, 0, 0). Extends ±6 in X, ±10 in Z.
 * Windmill sits at (0, 0, 2) — middle of course, forward of centre.
 * Tunnel through south wall of windmill base along Z axis.
 */

void buildCourse() {
    // ---- Ground surface ----
    add(new Mesh(makeCuboid(12.0f, 0.2f, 20.0f, cGrass)),
        mat4::translate(0, -0.1f, 0));

    // ---- Boundary walls ----
    // North wall
    add(new Mesh(makeCuboid(12.0f, 0.6f, 0.3f, cWall)),
        mat4::translate(0, 0.3f, -10.0f));
    // South wall – split into two pieces to leave tunnel gap (1.2 units wide)
    add(new Mesh(makeCuboid(4.9f, 0.6f, 0.3f, cWall)),
        mat4::translate(-3.55f, 0.3f, 10.0f));
    add(new Mesh(makeCuboid(4.9f, 0.6f, 0.3f, cWall)),
        mat4::translate( 3.55f, 0.3f, 10.0f));
    // East wall
    add(new Mesh(makeCuboid(0.3f, 0.6f, 20.0f, cWall)),
        mat4::translate(6.0f, 0.3f, 0));
    // West wall
    add(new Mesh(makeCuboid(0.3f, 0.6f, 20.0f, cWall)),
        mat4::translate(-6.0f, 0.3f, 0));

    // ---- Starting mat (south end, distinct yellow) ----
    add(new Mesh(makeCuboid(1.4f, 0.05f, 1.4f, cStartMat)),
        mat4::translate(0, 0.11f, 8.5f));

    // ---- Golf hole (cylinder, spec-required) ----
    add(new Mesh(makeCylinder(0.25f, 0.15f, 16, cHole)),
        mat4::translate(-2.5f, 0.0f, -7.5f));

    // ---- Golf ball (sphere, bonus spec item) ----
    // Starts at starting mat
    add(new Mesh(makeSphere(0.18f, 10, 10, cBall)),
        mat4::translate(0, 0.18f, 8.5f));

    // ---- Decorations outside boundary: 2 trees ----
    // Tree 1 – east outside
    add(new Mesh(makeCylinder(0.18f, 1.2f, 8, cTreeTrunk)),
        mat4::translate(7.5f, 0.0f, -3.0f));
    add(new Mesh(makeCone(0.65f, 2.0f, 8, cTreeTop)),
        mat4::translate(7.5f, 1.2f, -3.0f));

    // Tree 2 – west outside
    add(new Mesh(makeCylinder(0.18f, 1.2f, 8, cTreeTrunk)),
        mat4::translate(-7.5f, 0.0f, -3.0f));
    add(new Mesh(makeCone(0.65f, 2.0f, 8, cTreeTop)),
        mat4::translate(-7.5f, 1.2f, -3.0f));

    // ---- Pink flowers (bonus) – inside course near walls ----
    auto makeFlower = [&](float x, float z) {
        // Stem
        add(new Mesh(makeCylinder(0.04f, 0.4f, 8, cStem)),
            mat4::translate(x, 0.0f, z));
        // Head (small sphere)
        add(new Mesh(makeSphere(0.15f, 8, 8, cFlowerPink)),
            mat4::translate(x, 0.55f, z));
    };
    makeFlower( 4.5f,  6.0f);
    makeFlower(-4.5f,  6.0f);
    makeFlower( 4.5f, -6.0f);
}

// ---------------------------------------------------------------------------
// Windmill construction
// ---------------------------------------------------------------------------
/*
 * All windmill parts are placed relative to a local origin at the
 * centre of the windmill base (0, 0, 2) in world space.
 * We pass mat4::translate(0, 0, 2) * shapeLocal as each part's matrix.
 *
 * WINDMILL LAYOUT (Y = height above course ground):
 *   y=0.00 – ground level
 *   y=0.00 – base platform top face
 *   y=0.30 – tower starts
 *   y=0.50 – tunnel floor (ball rolls through here)
 *   y=4.20 – balcony ring
 *   y=4.80 – cap starts
 *   y=5.50 – cap ends / cone roof starts
 *   y=6.50 – cone apex
 *   y=5.80 – axle protrudes forward (z+0.5)
 *
 * TUNNEL: The base frustum has rBot=1.3, gap cut by leaving the front
 * face of the base open (the frustum geometry doesn't cover it).
 * Two wall cuboids flank the gap in the south wall of the base.
 *
 * BLADES: 4 blades at 90° apart, each a frame cuboid + sail cuboid.
 * They spin around the axle (BLADE_PIVOT, BLADE_AXIS=Z).
 */
void buildWindmill() {
    // Windmill world offset
    auto W = [](Mat4 local) -> Mat4 {
        return mat4::translate(0, 0, 2.0f) * local;
    };

    // ---- Base platform (raised octagonal frustum) ----
    add(new Mesh(makeFrustumColoured(1.8f, 1.6f, 0.3f, 8, cWood, cWood)),
        W(mat4::translate(0,0,0)));

    // ---- 4 diagonal support struts (triangular prisms) ----
    // These lean out from the base – represented as thin triangular prisms
    // rotated to angle outward, one on each cardinal side
    {
        float strutAngles[4] = {0, (float)M_PI/2, (float)M_PI, 3*(float)M_PI/2};
        for (int i=0; i<4; i++) {
            float a = strutAngles[i];
            // Strut leans outward at 30 degrees from vertical
            Mat4 local = mat4::translate(cosf(a)*1.1f, 0, sinf(a)*1.1f)
                       * mat4::rotateY(-a)
                       * mat4::rotateZ((float)M_PI/6.0f);  // lean 30 deg
            add(new Mesh(makeTriangularPrism(0.12f, 1.2f, 0.12f, cWood)),
                W(local));
        }
    }

    //Tower body
    add(new Mesh(makeFrustumColoured(1.3f, 0.7f, 4.2f, 8, cWhite, cDarkGreen)),
        W(mat4::translate(0, 0.3f, 0)));

    //dark green trim
    {
        float trimAngles[4] = {0, (float)M_PI/2, (float)M_PI, 3*(float)M_PI/2};
        for (int i=0; i<4; i++) {
            float a = trimAngles[i];
            float r = 0.72f; //approx radius at mid-tower
            add(new Mesh(makeCuboid(0.08f, 4.0f, 0.08f, cDarkGreen)),
                W(mat4::translate(cosf(a)*r, 0.5f, sinf(a)*r)));
        }
    }

    //balcony ring
    add(new Mesh(makeFrustumColoured(0.9f, 0.95f, 0.18f, 8, cWhite, cWhite)),
        W(mat4::translate(0, 4.2f, 0)));

    //balcony posts
    {
        for (int i=0; i<8; i++){
            float a = i * 2.0f*(float)M_PI/8;
            float r = 0.90f;
            add(new Mesh(makeCuboid(0.05f, 0.4f, 0.05f, cWhite)),
                W(mat4::translate(cosf(a)*r, 4.38f, sinf(a)*r)));
        }
    }

    add(new Mesh(makeFrustumColoured(1.9f, 1.95f, 0.18f, 12, cWood, cWood)),
        W(mat4::translate(0, 2.2f, 0)));

    {
        for (int i=0; i<12; i++){
            float a = i * 2.0f*(float)M_PI/8;
            float r = 1.85f;
            add(new Mesh(makeCuboid(0.05f, 3.0f, 0.05f, cWood)),
                W(mat4::translate(cosf(a)*r, 1.3f, sinf(a)*r)));
        }
    }

    //tower cap
    add(new Mesh(makeFrustumColoured(0.72f, 0.55f, 0.7f, 8, cDarkGreen, cDarkGreen)),
        W(mat4::translate(0, 4.5f, 0)));

    //roof cone
    add(new Mesh(makeCone(0.55f, 1.1f, 8, cDarkGreen)),
        W(mat4::translate(0, 5.2f, 0)));

    //axle cylinder
    add(new Mesh(makeCylinderColoured(0.10f, 1.0f, 12, cMetal, cMetal)),
        W(mat4::translate(0, 5.8f, -0.1f)   // centre it
          * mat4::rotateX((float)M_PI/2)));  // rotate so axis points +Z

    //rotor centre
    add(new Mesh(makeCylinder(0.22f, 0.15f, 12, cMetal)),
        W(mat4::translate(0, 5.73f, 0.43f)
          * mat4::rotateX((float)M_PI/2)));


    //the 4 blades are at 0°, 90°, 180°, 270° initially.
    {
        for (int i=0; i<4; i++) {
            float initialAngle = i * (float)M_PI / 2.0f;

            //each blade is positioned at the rotor and pivots out
            Mat4 bladeBase =
                mat4::translate(BLADE_PIVOT.x, BLADE_PIVOT.y, BLADE_PIVOT.z)
              * mat4::rotateZ(initialAngle)
              * mat4::translate(0, 0.0f, 0);  // root at pivot

            add(new Mesh(makeBlade(2.8f, 0.14f, 0.06f, cBladeFrame)),
                W(bladeBase), true);

            add(new Mesh(makeBlade(2.6f, 0.55f, 0.025f, cBladeSail)),
                W(mat4::translate(BLADE_PIVOT.x, BLADE_PIVOT.y,
                                  BLADE_PIVOT.z + 0.04f)
                  * mat4::rotateZ(initialAngle)
                  * mat4::translate(0.0f, 0.1f, 0)), true);
        }
    }

    //tunnel entrance
    add(new Mesh(makeCuboid(0.25f, 0.7f, 0.25f, cWood)),
        W(mat4::translate(-0.65f, 0.35f, 1.25f)));
    add(new Mesh(makeCuboid(0.25f, 0.7f, 0.25f, cWood)),
        W(mat4::translate( 0.65f, 0.35f, 1.25f)));
}

const char *getError()
{
    const char *errorDescription;
    glfwGetError(&errorDescription);
    return errorDescription;
}

inline void startUpGLFW()
{
    glewExperimental = true; // Needed for core profile
    if (!glfwInit())
    {
        throw getError();
    }
}

inline void startUpGLEW()
{
    glewExperimental = true; // Needed in core profile
    if (glewInit() != GLEW_OK)
    {
        glfwTerminate();
        throw getError();
    }
}

GLFWwindow* setUp() {
    startUpGLFW();
    glfwWindowHint(GLFW_SAMPLES, 4);               // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL
    GLFWwindow *window;                                            // (In the accompanying source code, this variable is global for simplicity)
    window = glfwCreateWindow(WIN_W, WIN_H, "u23718146", nullptr, nullptr);
    if (window == NULL)
    {
        cout << getError() << endl;
        glfwTerminate();
        throw "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n";
    }
    glfwMakeContextCurrent(window); // Initialize GLEW
    startUpGLEW();
    return window;
}

//callback for key presses
const float ROT_STEP   = 0.04f;
const float TRANS_STEP = 0.15f;

void keyCallback(GLFWwindow* win, int key, int, int action, int) {
    if (action == GLFW_RELEASE) return;

    switch(key) {
        // World rotations (pre-multiply = world-space axes stay fixed)
        case GLFW_KEY_W: worldMatrix = mat4::rotateX(-ROT_STEP)*worldMatrix; break;
        case GLFW_KEY_S: worldMatrix = mat4::rotateX( ROT_STEP)*worldMatrix; break;
        case GLFW_KEY_A: worldMatrix = mat4::rotateY(-ROT_STEP)*worldMatrix; break;
        case GLFW_KEY_D: worldMatrix = mat4::rotateY( ROT_STEP)*worldMatrix; break;
        case GLFW_KEY_E: worldMatrix = mat4::rotateZ(-ROT_STEP)*worldMatrix; break;
        case GLFW_KEY_Q: worldMatrix = mat4::rotateZ( ROT_STEP)*worldMatrix; break;

        // World translations
        case GLFW_KEY_I: worldMatrix = mat4::translate(0, TRANS_STEP,0)*worldMatrix; break;
        case GLFW_KEY_K: worldMatrix = mat4::translate(0,-TRANS_STEP,0)*worldMatrix; break;
        case GLFW_KEY_L: worldMatrix = mat4::translate( TRANS_STEP,0,0)*worldMatrix; break;
        case GLFW_KEY_J: worldMatrix = mat4::translate(-TRANS_STEP,0,0)*worldMatrix; break;
        case GLFW_KEY_O: worldMatrix = mat4::translate(0,0, TRANS_STEP)*worldMatrix; break;
        case GLFW_KEY_U: worldMatrix = mat4::translate(0,0,-TRANS_STEP)*worldMatrix; break;

        // Blade speed
        case GLFW_KEY_EQUAL:
        case GLFW_KEY_KP_ADD:
            bladeSpeed += 0.4f;
            break;
        case GLFW_KEY_MINUS:
        case GLFW_KEY_KP_SUBTRACT:
            bladeSpeed -= 0.4f;
            if (bladeSpeed < 0.0f) bladeSpeed = 0.0f;
            break;

        // Wireframe toggle (debounced)
        case GLFW_KEY_ENTER:
            if (action == GLFW_PRESS) {
                double now = glfwGetTime();
                if (now - lastEnter > 0.25) {
                    wireframe = !wireframe;
                    lastEnter = now;
                }
            }
            break;

        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(win, GLFW_TRUE);
            break;
        default: break;
    }
}

int main() {
    GLFWwindow *window;
    try
    {
        window = setUp();
    }
    catch (const char *e)
    {
        cout << e << endl;
        throw;
    }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetKeyCallback(window, keyCallback);

    GLuint prog = LoadShaders("vertex3d.glsl", "fragment3d.glsl");
    if (!prog) { cerr << "Shader load failed\n"; glfwTerminate(); return -1; }

    GLint uModel      = glGetUniformLocation(prog, "model");
    GLint uView       = glGetUniformLocation(prog, "view");
    GLint uProjection = glGetUniformLocation(prog, "projection");

    //build scene
    buildCourse();
    buildWindmill();

    //camera init
    Vec3 eye    = {0.0f,  12.0f, 22.0f};
    Vec3 target = {0.0f,   2.0f,  0.0f};
    Vec3 up     = {0.0f,   1.0f,  0.0f};
    Mat4 view   = mat4::lookAt(eye, target, up);

    //3d projection
    float fovY   = (float)M_PI / 4.0f;  //45 degrees
    float aspect = (float)WIN_W / WIN_H;
    Mat4  proj   = mat4::perspective(fovY, aspect, 0.1f, 200.0f);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.40f, 0.65f, 0.90f, 1.0f); //blue background

    lastTime = glfwGetTime();

    //main render loop
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float  dt  = (float)(now - lastTime);
        lastTime   = now;

        //make spin if need spin
        bladeAngle += bladeSpeed * dt;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog);
        glUniformMatrix4fv(uView,       1, GL_FALSE, view.ptr());
        glUniformMatrix4fv(uProjection, 1, GL_FALSE, proj.ptr());


         //For each blade object, spinMat = T(+pivot) * R(bladeAngle, Z) * T(-pivot)
         //the blades initialAngle is already baked into its localMatrix
        Mat4 pivotFwd = mat4::translate( BLADE_PIVOT.x,
                                         BLADE_PIVOT.y,
                                         BLADE_PIVOT.z + 2.0f); // +2 for wm offset
        Mat4 pivotBck = mat4::translate(-BLADE_PIVOT.x,
                                        -BLADE_PIVOT.y,
                                        -BLADE_PIVOT.z - 2.0f);
        Mat4 spinMat  = pivotFwd
                      * mat4::rotateZ(bladeAngle)
                      * pivotBck;

        for (auto& obj : scene) {
            Mat4 model;
            if (obj.isBlade) {
                model = worldMatrix * spinMat * obj.local;
            } else {
                model = worldMatrix * obj.local;
            }
            glUniformMatrix4fv(uModel, 1, GL_FALSE, model.ptr());
            obj.mesh->draw(wireframe);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //cleanup
    for (auto& obj : scene) delete obj.mesh;
    glDeleteProgram(prog);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
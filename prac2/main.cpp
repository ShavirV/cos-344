/*
 * main.cpp - COS344 Practical 2: Mini-Golf Course
 * =================================================
 * Student Number: u00000000  <-- replace this
 *
 * ARCHITECTURE OVERVIEW
 * ---------------------
 * All shapes are stored as Shape<3>* pointers in a flat vector.
 * The template parameter 3 = homogeneous 2D: (x, y, w) where w=1.
 *
 * Each shape owns its own VAO/VBO pair. On every frame we:
 *   1. Call shape->getPoints() to get the current (already-transformed) vertices
 *   2. Upload to the VBO (GL_STREAM_DRAW - data changes every frame)
 *   3. Draw with the appropriate primitive
 *
 * IMPORTANT ASSUMPTION about Matrix<3,3> default constructor:
 *   The move/scale/rotate implementations assume Matrix<3,3>() gives the
 *   IDENTITY matrix. If your Matrix default-constructs to zeros, you need
 *   to call an explicit identity() factory instead. Check your Matrix.h.
 *
 * COLOUR STRATEGY
 * ---------------
 * The vertex shader receives per-vertex colour as a vec3 attribute.
 * Each shape has a "normal" colour (RGB floats) and a "selected" colour
 * (pastel version). When a shape is selected, we pass the pastel colour.
 * This is managed by ShapeRenderer::setColour().
 *
 * WIREFRAME (GL_LINES, NOT glPolygonMode)
 * ----------------------------------------
 * For each shape, getPoints() returns the primitive's raw vertices.
 *   - GL_TRIANGLE_FAN shapes (Polygon): convert each triangle slice into
 *     3 line segments (6 vertices) for GL_LINES.
 *   - GL_TRIANGLE_FAN shapes (Square): 4-vertex fan -> 4 edges.
 *   - GL_TRIANGLES (Triangle): 3 vertices -> 3 edges.
 *
 * SCENE LAYOUT (top-down mini-golf hole)
 * ----------------------------------------
 *   [  BACKGROUND (dark grey)                              ]
 *   [  CONCRETE FLOOR (grey, inset)                        ]
 *   [  GRASS (green)                                       ]
 *   [  BARRIER walls x4  |  RIVER (blue rect)              ]
 *   [  START block (maroon) | LOGS x2 (brown rects)        ]
 *   [  RAMP triangles x2  |  BUSH octagon x2               ]
 *   [  GOLF BALL (circle, 60 seg)  |  HOLE (circle, 60seg) ]
 *
 * SELECTABLE OBJECTS (keys 1-4)
 *   1 = golf ball  (Polygon<3>, smooth circle)
 *   2 = log obstacle  (Square<3>)
 *   3 = ramp obstacle  (Triangle<3>)
 *   4 = golf hole  (Polygon<3>, smooth circle)
 *
 * CONTROLS
 *   1-4    select shape        0       deselect
 *   WASD   translate           +/-     scale
 *   L/R    rotate              Enter   toggle wireframe
 *   Esc    quit
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "shader.hpp"
#include "Shape.h"
#include "Square.h"
#include "Triangle.h"
#include "Polygon.h"
#include "Vector.h"
#include "Matrix.h"

using namespace std;

// ---------------------------------------------------------------------------
// Window
// ---------------------------------------------------------------------------
const int WIN_W = 1000;
const int WIN_H = 1000;

// ---------------------------------------------------------------------------
// Colours (RGB float triples for each shape)
// We store normal and pastel (selected) variants per-shape index.
// ---------------------------------------------------------------------------
struct RGB { float r, g, b; };

// Pastel: blend t=0.55 toward white
RGB pastel(RGB c) {
    float t = 0.55f;
    return { c.r + t*(1-c.r), c.g + t*(1-c.g), c.b + t*(1-c.b) };
}

// ---------------------------------------------------------------------------
// ShapeRenderer
// Wraps a Shape<3>* with GPU objects and colour metadata.
// ---------------------------------------------------------------------------

/*
 * Primitive type tells the renderer how to draw and how to build wireframe.
 *   TRIFAN   - GL_TRIANGLE_FAN  (Polygon, Square)
 *   TRILIST  - GL_TRIANGLES     (Triangle)
 */
enum class Prim { TRIFAN, TRILIST };

struct ShapeRenderer {
    Shape<3>*  shape;
    GLuint     vao, vbo;
    RGB        normalColour;
    RGB        selectedColour;
    Prim       prim;
    int        vertexCount; // number of VERTICES (not floats) for the draw call

    // Build a flat interleaved buffer: [x y w  r g b]  per vertex (6 floats)
    // The `w` homogeneous coord is stripped; we only send x,y to the shader.
    void upload(bool wireframe, bool selected) {
        float* pts = shape->getPoints();  // returns n*vertexCount floats (n=3)
        RGB col = selected ? selectedColour : normalColour;

        // For wireframe we expand triangles into line pairs
        vector<float> buf;

        if (!wireframe) {
            // Filled: just pack x,y,r,g,b per vertex (skip w at index 2)
            for (int i = 0; i < vertexCount; i++) {
                buf.push_back(pts[i*3 + 0]);  // x
                buf.push_back(pts[i*3 + 1]);  // y
                buf.push_back(col.r);
                buf.push_back(col.g);
                buf.push_back(col.b);
            }
        } else {
            // Wireframe: convert each triangle to 3 line segments (6 vertices)
            // We need to reconstruct the triangle list first.
            // For TRIFAN: triangle i = {fan[0], fan[i], fan[i+1]}
            // For TRILIST: triangle i = {list[i*3], list[i*3+1], list[i*3+2]}

            auto pushV = [&](int idx) {
                buf.push_back(pts[idx*3 + 0]);
                buf.push_back(pts[idx*3 + 1]);
                buf.push_back(col.r);
                buf.push_back(col.g);
                buf.push_back(col.b);
            };
            auto pushEdge = [&](int a, int b) {
                pushV(a); pushV(b);
            };

            if (prim == Prim::TRIFAN) {
                // Fan has vertexCount-2 triangles
                // Triangle i: vertex 0, vertex i+1, vertex i+2
                int triCount = vertexCount - 2;
                for (int i = 0; i < triCount; i++) {
                    int a = 0, b = i+1, c = i+2;
                    pushEdge(a, b);
                    pushEdge(b, c);
                    pushEdge(c, a);
                }
            } else {
                // TRILIST: every 3 vertices form one triangle
                int triCount = vertexCount / 3;
                for (int i = 0; i < triCount; i++) {
                    int a = i*3, b = i*3+1, c = i*3+2;
                    pushEdge(a, b);
                    pushEdge(b, c);
                    pushEdge(c, a);
                }
            }
        }

        delete[] pts;

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     buf.size() * sizeof(float),
                     buf.data(), GL_STREAM_DRAW);

        // Attribute 0: position (x, y)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              5*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Attribute 1: colour (r, g, b)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              5*sizeof(float), (void*)(2*sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void draw(bool wireframe, bool selected) {
        upload(wireframe, selected);
        glBindVertexArray(vao);

        if (!wireframe) {
            GLenum mode = (prim == Prim::TRIFAN) ? GL_TRIANGLE_FAN : GL_TRIANGLES;
            glDrawArrays(mode, 0, vertexCount);
        } else {
            // Each triangle -> 3 edges -> 6 vertices
            int triCount = (prim == Prim::TRIFAN)
                           ? vertexCount - 2
                           : vertexCount / 3;
            glDrawArrays(GL_LINES, 0, triCount * 6);
        }

        glBindVertexArray(0);
    }
};

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
vector<ShapeRenderer> renderers;  // all renderable shapes, back-to-front order
int selectedIdx   = -1;           // index into renderers, -1 = none
bool wireframe    = false;
double lastEnter  = 0.0;          // debounce Enter key

// Indices of the 4 selectable shapes
int idxBall = -1, idxLog = -1, idxRamp = -1, idxHole = -1;

// ---------------------------------------------------------------------------
// Helper: make a ShapeRenderer and add it to the global list
// Returns the index it was added at.
// ---------------------------------------------------------------------------
int addShape(Shape<3>* s, Prim prim, int vertexCount, RGB col) {
    ShapeRenderer sr;
    sr.shape         = s;
    sr.prim          = prim;
    sr.vertexCount   = vertexCount;
    sr.normalColour  = col;
    sr.selectedColour = pastel(col);
    glGenVertexArrays(1, &sr.vao);
    glGenBuffers(1, &sr.vbo);
    int idx = (int)renderers.size();
    renderers.push_back(sr);
    return idx;
}

// Helper to make a Vector<3> for homogeneous 2D: (x, y, 1)
Vector<3> v3(float x, float y) {
    Vector<3> v;
    v[0] = x; v[1] = y; v[2] = 1.0f;
    return v;
}

// ---------------------------------------------------------------------------
// buildScene - construct all golf course shapes
// ---------------------------------------------------------------------------
/*
 * Coordinate space: OpenGL NDC, x and y in [-1, 1].
 * Origin at centre of screen. Y up.
 *
 * Scene layers (drawn back to front = painter's algorithm):
 *   0. Background:  giant grey rect covering whole screen
 *   1. Concrete:    grey rect inset
 *   2. Grass:       green rect inset further
 *   3. Barriers:    4 dark-brown rects along the grass edge
 *   4. River:       blue rect
 *   5. Start block: maroon rect
 *   6. Logs:        2 sienna rects  [one is selectable as obj 2]
 *   7. Ramps:       2 red triangles [one is selectable as obj 3]
 *   8. Bushes:      2 dark-green 8-gons (low-poly circles)
 *   9. Golf hole:   pink 60-gon    [selectable as obj 4]
 *  10. Golf ball:   white 60-gon   [selectable as obj 1]
 */
void buildScene() {
    // ---- Colours ----
    RGB cConcrete = {0.50f, 0.50f, 0.50f};
    RGB cGrass    = {0.13f, 0.55f, 0.13f};
    RGB cBorder   = {0.30f, 0.18f, 0.07f};
    RGB cStart    = {0.55f, 0.00f, 0.00f};
    RGB cRiver    = {0.10f, 0.30f, 0.80f};
    RGB cLog      = {0.65f, 0.35f, 0.10f};
    RGB cRamp     = {0.80f, 0.10f, 0.10f};
    RGB cBush     = {0.00f, 0.39f, 0.00f};
    RGB cHole     = {1.00f, 0.41f, 0.71f};
    RGB cBall     = {1.00f, 1.00f, 1.00f};

    // ---- Concrete floor ----
    // Square(centre, height, width)
    addShape(new Square<3>(v3(0,0), 1.74f, 1.74f), Prim::TRIFAN, 4, cConcrete);

    // ---- Grass ----
    addShape(new Square<3>(v3(0,0), 1.56f, 1.56f), Prim::TRIFAN, 4, cGrass);

    // ---- 4 border barriers (dark brown, along grass edge) ----
    addShape(new Square<3>(v3( 0.00f,  0.81f), 0.11f, 1.74f), Prim::TRIFAN, 4, cBorder); // top
    addShape(new Square<3>(v3( 0.00f, -0.81f), 0.11f, 1.74f), Prim::TRIFAN, 4, cBorder); // bottom
    addShape(new Square<3>(v3(-0.81f,  0.00f), 1.74f, 0.11f), Prim::TRIFAN, 4, cBorder); // left
    addShape(new Square<3>(v3( 0.81f,  0.00f), 1.74f, 0.11f), Prim::TRIFAN, 4, cBorder); // right

    // ---- River (blue vertical rect) ----
    addShape(new Square<3>(v3(-0.10f, 0.00f), 1.20f, 0.20f), Prim::TRIFAN, 4, cRiver);

    // ---- Start block ----
    addShape(new Square<3>(v3(-0.62f, 0.00f), 0.28f, 0.18f), Prim::TRIFAN, 4, cStart);

    // ---- Logs (obstacle type 1) - one selectable ----
    idxLog = addShape(new Square<3>(v3(0.30f,  0.35f), 0.12f, 0.44f), Prim::TRIFAN, 4, cLog);
             addShape(new Square<3>(v3(0.30f, -0.10f), 0.12f, 0.44f), Prim::TRIFAN, 4, cLog);

    // ---- Ramps (obstacle type 2, triangles) - one selectable ----
    //   Triangle: apex at top, base at bottom
    idxRamp = addShape(
        new Triangle<3>(v3(0.50f,  0.65f), v3(0.35f, 0.20f), v3(0.65f, 0.20f)),
        Prim::TRILIST, 3, cRamp);
              addShape(
        new Triangle<3>(v3(0.72f, -0.30f), v3(0.58f,-0.60f), v3(0.86f,-0.60f)),
        Prim::TRILIST, 3, cRamp);

    // ---- Bushes (low-poly circles, 8 sides = octagon) ----
    addShape(new Polygon<3>(v3(-0.50f,  0.50f), 0.09f, 8), Prim::TRIFAN, 10, cBush);
    addShape(new Polygon<3>(v3(-0.50f, -0.50f), 0.09f, 8), Prim::TRIFAN, 10, cBush);

    // ---- Golf hole (smooth circle, 60 sides) ----
    // 60 sides + centre + closing = 62 vertices
    idxHole = addShape(new Polygon<3>(v3(0.60f, -0.55f), 0.08f, 60),
                       Prim::TRIFAN, 62, cHole);

    // ---- Golf ball (smooth circle, 60 sides) ----
    idxBall = addShape(new Polygon<3>(v3(-0.62f, 0.22f), 0.05f, 60),
                       Prim::TRIFAN, 62, cBall);
}

// ---------------------------------------------------------------------------
// Selection helper
// ---------------------------------------------------------------------------
void selectShape(int idx) {
    // Deselect current
    if (selectedIdx >= 0)
        renderers[selectedIdx].shape->deselect();

    selectedIdx = idx;

    if (idx >= 0)
        renderers[idx].shape->select();
}

// ---------------------------------------------------------------------------
// GLFW setup helpers (from template)
// ---------------------------------------------------------------------------
const char* getError() {
    const char* d; glfwGetError(&d); return d;
}
inline void startUpGLFW() {
    glewExperimental = true;
    if (!glfwInit()) throw getError();
}
inline void startUpGLEW() {
    glewExperimental = true;
    if (glewInit() != GLEW_OK) { glfwTerminate(); throw getError(); }
}
inline GLFWwindow* setUp() {
    startUpGLFW();
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* w = glfwCreateWindow(WIN_W, WIN_H, "u00000000", NULL, NULL);
    if (!w) { cout << getError(); glfwTerminate(); throw "Window failed"; }
    glfwMakeContextCurrent(w);
    startUpGLEW();
    return w;
}

// ---------------------------------------------------------------------------
// Key callback
// ---------------------------------------------------------------------------
/*
 * Selection keys act on PRESS only.
 * Transform keys act on PRESS and REPEAT (held down).
 * Enter is debounced (250ms) to prevent toggling twice on one press.
 */
void keyCallback(GLFWwindow* win, int key, int /*sc*/, int action, int /*mods*/) {
    if (action == GLFW_RELEASE) return;

    // ---- Selection (press only) ----
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_1: selectShape(idxBall); return;
            case GLFW_KEY_2: selectShape(idxLog);  return;
            case GLFW_KEY_3: selectShape(idxRamp); return;
            case GLFW_KEY_4: selectShape(idxHole); return;
            case GLFW_KEY_0: selectShape(-1);       return;

            case GLFW_KEY_ENTER: {
                // Debounced wireframe toggle (spec hint: use a time delay)
                double now = glfwGetTime();
                if (now - lastEnter > 0.25) {
                    wireframe = !wireframe;
                    lastEnter = now;
                }
                return;
            }
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(win, GLFW_TRUE);
                return;
            default: break;
        }
    }

    // ---- Transforms (press + repeat, only when selected) ----
    if (selectedIdx < 0) return;
    Shape<3>* s = renderers[selectedIdx].shape;

    switch (key) {
        // WASD = translate
        case GLFW_KEY_W: s->move('w'); break;
        case GLFW_KEY_S: s->move('s'); break;
        case GLFW_KEY_A: s->move('a'); break;
        case GLFW_KEY_D: s->move('d'); break;

        // +/- = scale (EQUAL = '+' on most keyboards)
        case GLFW_KEY_EQUAL:
        case GLFW_KEY_KP_ADD:    s->scale('+'); break;
        case GLFW_KEY_MINUS:
        case GLFW_KEY_KP_SUBTRACT: s->scale('-'); break;

        // L/R arrow = rotate
        // Spec says rotate left/right - we map arrow keys and L/R letters
        case GLFW_KEY_LEFT:
        case GLFW_KEY_Q: s->rotate('l'); break;
        case GLFW_KEY_RIGHT:
        case GLFW_KEY_E: s->rotate('r'); break;

        default: break;
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    GLFWwindow* window;
    try { window = setUp(); }
    catch (const char* e) { cout << e << endl; return -1; }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetKeyCallback(window, keyCallback);

    // Load shaders - vertex.glsl and fragment.glsl must be in the same directory
    GLuint prog = LoadShaders("vertex.glsl", "fragment.glsl");
    if (prog == 0) { cerr << "Shader load failed\n"; glfwTerminate(); return -1; }

    buildScene();

    // Dark background so all shape colours are visible
    glClearColor(0.08f, 0.08f, 0.08f, 1.0f);

    // ---- Render loop ----
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);

        // Draw all shapes back to front (painter's algorithm)
        for (int i = 0; i < (int)renderers.size(); i++) {
            renderers[i].draw(wireframe, i == selectedIdx);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ---- Cleanup ----
    for (auto& r : renderers) {
        delete r.shape;
        glDeleteVertexArrays(1, &r.vao);
        glDeleteBuffers(1, &r.vbo);
    }
    glDeleteProgram(prog);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
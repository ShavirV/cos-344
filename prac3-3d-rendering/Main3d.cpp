/*
 * main3d.cpp  –  COS344 Practical 3: 3D Shape Test
 * ==================================================
 * u23718146  –  Shavir Vallabh
 *
 * PURPOSE
 * -------
 * Demonstrates all required 3D primitives and the MVP pipeline.
 * This is the TEST MAIN — the full windmill scene will be built
 * on top of this foundation.
 *
 * WHAT IS ON SCREEN
 * -----------------
 * Arranged in a grid so you can see every shape at once:
 *
 *   [Cuboid]   [TriPrism]   [Cylinder]   [Cone]   [Blade]
 *
 * CONTROLS
 * --------
 *   W / S      – rotate world around X axis
 *   A / D      – rotate world around Y axis
 *   Q / E      – rotate world around Z axis
 *   I / K      – translate Y
 *   J / L      – translate X
 *   U / O      – translate Z
 *   Enter      – toggle wireframe (debounced 250ms)
 *   Escape     – quit
 *
 * ARCHITECTURE
 * ------------
 * All shapes share one shader program.
 * Each shape is a Mesh with its own VAO/VBO.
 * The MODEL matrix per shape positions it in the world.
 * A global WORLD matrix accumulates WASD rotations and IJKL translations —
 * every shape's final matrix = worldMatrix * shapeLocalMatrix.
 * This means pressing W once moves everything together, which is exactly
 * what the spec requires for P3 (windmill + course rotate together).
 *
 * MATRIX PIPELINE
 * ---------------
 *   projection  = perspective(45°, aspect, 0.1, 100)       -- built once
 *   view        = lookAt(eye, origin, up)                   -- built once
 *   world       = accumulated rotations + translations      -- updated on keypress
 *   model       = world * shapeLocal                        -- per shape per frame
 *
 *   GPU receives:  projection * view * model
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "shader.hpp"
#include "Mat4.hpp"
#include "Mesh.hpp"
#include "Shapes3d.hpp"

using namespace std;

// ---------------------------------------------------------------------------
// Window
// ---------------------------------------------------------------------------
const int WIN_W = 1000;
const int WIN_H = 1000;

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
bool wireframe  = false;
double lastEnter = 0.0;

/*
 * worldMatrix accumulates ALL user-applied rotations and translations.
 * Every shape multiplies its local model matrix by this on the left:
 *   finalModel = worldMatrix * localModel
 *
 * This satisfies the spec rule: "transforms must not reset to centre
 * before a new transformation is applied."
 * The matrix is never reset — it only ever gets pre-multiplied.
 */
Mat4 worldMatrix; // starts as identity

// Animation state (blade spin — tested here, used fully in P3 windmill)
float bladeSpeed = 0.0f;  // radians per second
float bladeAngle = 0.0f;  // accumulated angle
double lastTime  = 0.0;

// ---------------------------------------------------------------------------
// Scene objects
// ---------------------------------------------------------------------------
// Each entry: a Mesh + its local-space placement matrix
struct SceneObject {
    Mesh*  mesh;       // pointer so we can store in vector after move
    Mat4   localModel; // local position/orientation within the world
};

vector<SceneObject> objects;
int bladeObjIdx = -1; // index of the animated blade in objects[]

// ---------------------------------------------------------------------------
// GLFW helpers
// ---------------------------------------------------------------------------
const char* getGLFWError() {
    const char* d; glfwGetError(&d); return d;
}

GLFWwindow* createWindow() {
    glewExperimental = true;
    if (!glfwInit()) { cerr << getGLFWError(); return nullptr; }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    GLFWwindow* w = glfwCreateWindow(WIN_W, WIN_H, "u23718146", nullptr, nullptr);
    if (!w) { cerr << getGLFWError(); glfwTerminate(); return nullptr; }

    glfwMakeContextCurrent(w);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        cerr << "GLEW init failed\n"; glfwTerminate(); return nullptr;
    }
    return w;
}

// ---------------------------------------------------------------------------
// Key callback
// ---------------------------------------------------------------------------
/*
 * Rotation step: small enough to see individual key presses clearly.
 * Translation step: moves at a comfortable pace.
 *
 * HOW WORLD MATRIX ACCUMULATES
 * ----------------------------
 * Each key press PRE-MULTIPLIES worldMatrix by the new transform:
 *   worldMatrix = newTransform * worldMatrix
 *
 * Pre-multiplying applies the transform in WORLD space (not local space),
 * which is what the spec examples show — pressing W always tilts the
 * scene toward the viewer regardless of prior rotations.
 */
const float ROT_STEP   = 0.05f;  // radians per keypress (~3 degrees)
const float TRANS_STEP = 0.1f;   // world units per keypress

void keyCallback(GLFWwindow* win, int key, int, int action, int) {
    if (action == GLFW_RELEASE) return;

    switch (key) {
        // ---- World rotations (WASD + QE) ----
        case GLFW_KEY_W: worldMatrix = mat4::rotateX(-ROT_STEP) * worldMatrix; break;
        case GLFW_KEY_S: worldMatrix = mat4::rotateX( ROT_STEP) * worldMatrix; break;
        case GLFW_KEY_A: worldMatrix = mat4::rotateY(-ROT_STEP) * worldMatrix; break;
        case GLFW_KEY_D: worldMatrix = mat4::rotateY( ROT_STEP) * worldMatrix; break;
        case GLFW_KEY_E: worldMatrix = mat4::rotateZ(-ROT_STEP) * worldMatrix; break;
        case GLFW_KEY_Q: worldMatrix = mat4::rotateZ( ROT_STEP) * worldMatrix; break;

        // ---- World translations (IJKL + UO) ----
        case GLFW_KEY_I: worldMatrix = mat4::translate( 0,  TRANS_STEP,  0) * worldMatrix; break;
        case GLFW_KEY_K: worldMatrix = mat4::translate( 0, -TRANS_STEP,  0) * worldMatrix; break;
        case GLFW_KEY_L: worldMatrix = mat4::translate( TRANS_STEP, 0,  0) * worldMatrix; break;
        case GLFW_KEY_J: worldMatrix = mat4::translate(-TRANS_STEP, 0,  0) * worldMatrix; break;
        case GLFW_KEY_O: worldMatrix = mat4::translate( 0,  0,  TRANS_STEP) * worldMatrix; break;
        case GLFW_KEY_U: worldMatrix = mat4::translate( 0,  0, -TRANS_STEP) * worldMatrix; break;

        // ---- Blade speed (+/-) ----
        // clamp at 0 so blades never spin backwards
        case GLFW_KEY_EQUAL:
        case GLFW_KEY_KP_ADD:      bladeSpeed += 0.5f; break;
        case GLFW_KEY_MINUS:
        case GLFW_KEY_KP_SUBTRACT:
            bladeSpeed -= 0.5f;
            if (bladeSpeed < 0.0f) bladeSpeed = 0.0f;
            break;

        // ---- Wireframe toggle ----
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

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------
/*
 * All shapes are placed along X at y=0, z=0, spaced 2.5 units apart.
 * The camera looks from above-and-front at the origin so all are visible.
 *
 * Mesh objects are heap-allocated and stored as pointers in SceneObject
 * so that moving them into the vector doesn't invalidate GPU handles.
 */
void buildScene() {
    // Colours
    RGB3 cStone  = {0.76f, 0.70f, 0.60f};  // sandstone
    RGB3 cBrick  = {0.78f, 0.35f, 0.25f};  // red brick
    RGB3 cMetal  = {0.55f, 0.60f, 0.65f};  // metallic grey
    RGB3 cGreen  = {0.20f, 0.65f, 0.20f};  // grass green
    RGB3 cBlade  = {0.90f, 0.90f, 0.90f};  // white blade

    // ---- 1. Cuboid ----
    {
        Mesh* mesh = new Mesh(makeCuboid(1.0f, 1.5f, 1.0f, cStone));
        Mat4 local = mat4::translate(-5.0f, 0.0f, 0.0f);
        objects.push_back({mesh, local});
    }

    // ---- 2. Triangular Prism (roof shape) ----
    {
        Mesh* mesh = new Mesh(makeTriangularPrism(1.2f, 1.0f, 1.2f, cBrick));
        Mat4 local = mat4::translate(-2.5f, 0.0f, 0.0f);
        objects.push_back({mesh, local});
    }

    // ---- 3. Cylinder (axle / hole) ----
    {
        Mesh* mesh = new Mesh(makeCylinder(0.5f, 1.5f, 16, cMetal));
        Mat4 local = mat4::translate(0.0f, 0.0f, 0.0f);
        objects.push_back({mesh, local});
    }

    // ---- 4. Cone (spire / roof tip) ----
    {
        Mesh* mesh = new Mesh(makeCone(0.6f, 1.5f, 12, cBrick));
        Mat4 local = mat4::translate(2.5f, 0.0f, 0.0f);
        objects.push_back({mesh, local});
    }

    // ---- 5. Blade (animated, spins around its root) ----
    {
        Mesh* mesh = new Mesh(makeBlade(1.5f, 0.3f, 0.08f, cBlade));
        // The blade's local position: attached at x=5, at height y=0.75
        // bladeAngle is applied as an additional rotation in the render loop
        Mat4 local = mat4::translate(5.0f, 0.0f, 0.0f);
        bladeObjIdx = (int)objects.size();
        objects.push_back({mesh, local});
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    GLFWwindow* window = createWindow();
    if (!window) return -1;

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetKeyCallback(window, keyCallback);

    // Load shaders
    GLuint prog = LoadShaders("vertex3d.glsl", "fragment3d.glsl");
    if (!prog) { cerr << "Shader load failed\n"; glfwTerminate(); return -1; }

    // Get uniform locations
    GLint uModel      = glGetUniformLocation(prog, "model");
    GLint uView       = glGetUniformLocation(prog, "view");
    GLint uProjection = glGetUniformLocation(prog, "projection");

    // Build scene geometry
    buildScene();

    // ---- Fixed camera (view matrix) ----
    // Eye slightly above and in front, looking at origin
    Vec3 eye    = {0.0f, 4.0f, 8.0f};
    Vec3 target = {0.0f, 0.0f, 0.0f};
    Vec3 up     = {0.0f, 1.0f, 0.0f};
    Mat4 view   = mat4::lookAt(eye, target, up);

    // ---- Perspective projection ----
    float fovY   = (float)M_PI / 4.0f;  // 45 degrees
    float aspect = (float)WIN_W / (float)WIN_H;
    Mat4 projection = mat4::perspective(fovY, aspect, 0.1f, 100.0f);

    // ---- OpenGL state ----
    glEnable(GL_DEPTH_TEST);
    // Sky-blue background — distinctive per spec
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

    lastTime = glfwGetTime();

    // ---- Render loop ----
    while (!glfwWindowShouldClose(window)) {
        // Delta time for smooth blade animation
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;

        // Update blade angle (clamp speed >= 0 enforced in keyCallback)
        bladeAngle += bladeSpeed * dt;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog);

        // Upload fixed matrices once per frame
        glUniformMatrix4fv(uView,       1, GL_FALSE, view.ptr());
        glUniformMatrix4fv(uProjection, 1, GL_FALSE, projection.ptr());

        // Draw each object
        for (int i = 0; i < (int)objects.size(); i++) {
            SceneObject& obj = objects[i];

            Mat4 model = worldMatrix * obj.localModel;

            // ---- Blade-specific animation ----
            // Rotate the blade around its root (Z axis through the origin
            // of its local space) by the accumulated bladeAngle.
            // T(pivot) * R(angle) * T(-pivot):
            // Since the blade root is at local origin (y=0), this is
            // simply R(angle) in local space, applied BEFORE localModel.
            if (i == bladeObjIdx) {
                Mat4 spin = mat4::rotateZ(bladeAngle);
                // Apply spin in LOCAL space: model = world * local * spin
                // But blade origin IS the root, so spin directly:
                model = worldMatrix * obj.localModel * spin;
            }

            glUniformMatrix4fv(uModel, 1, GL_FALSE, model.ptr());
            obj.mesh->draw(wireframe);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    for (auto& obj : objects) delete obj.mesh;
    glDeleteProgram(prog);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
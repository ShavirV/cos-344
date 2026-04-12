#ifndef SHAPES3D_HPP
#define SHAPES3D_HPP

/*
 * Shapes3D.hpp  –  Factory functions for all required 3D primitives
 * ==================================================================
 * Each function builds a Mesh from first principles (hand-coded geometry,
 * no OpenGL shape generators). All geometry is in LOCAL space — the model
 * matrix applied at draw time positions it in the world.
 *
 * Shapes provided:
 *   makeCuboid(w, h, d, col)            – axis-aligned box
 *   makeTriangularPrism(w, h, d, col)   – triangular cross-section prism
 *   makeCylinder(r, h, segs, col)       – cylinder with ≥8 segments
 *   makeCone(r, h, segs, col)           – cone with ≥8 segments
 *
 * All shapes are centred at the origin unless noted otherwise:
 *   Cuboid:          centre at origin, extends ±w/2, ±h/2, ±d/2
 *   TriangularPrism: base triangle in XZ plane centred at origin, extends up by h
 *   Cylinder:        axis along Y, base at y=0, top at y=h, centred on XZ
 *   Cone:            base at y=0, apex at y=h, centred on XZ
 *
 * WINDING ORDER
 * -------------
 * All triangles use CCW winding when viewed from OUTSIDE the shape,
 * so standard back-face culling (glEnable(GL_CULL_FACE)) works correctly.
 * The test main disables culling for simplicity.
 *
 * WIREFRAME
 * ---------
 * Mesh::addTriangle() automatically records edges for GL_LINES.
 * No separate wireframe geometry is needed here.
 */

#include "Mesh.hpp"
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// Helper: add a quad (two CCW triangles) facing outward
// v0,v1,v2,v3 should be in CCW order when viewed from outside
// ---------------------------------------------------------------------------
static void addQuad(Mesh& m, Vec3 v0, Vec3 v1, Vec3 v2, Vec3 v3, RGB3 col) {
    m.addTriangle(v0, v1, v2, col);
    m.addTriangle(v0, v2, v3, col);
}

// ---------------------------------------------------------------------------
// Cuboid
// ---------------------------------------------------------------------------
/*
 * Creates a box centred at origin.
 *   w = full width  (X axis)
 *   h = full height (Y axis)
 *   d = full depth  (Z axis)
 *
 * 6 faces × 2 triangles = 12 triangles total.
 * Each face can have a different colour for visual distinction —
 * pass a single colour and all faces share it (typical usage),
 * or call the overload below for per-face colours.
 *
 * Face normals (outward):
 *   +X right,  -X left
 *   +Y top,    -Y bottom
 *   +Z front,  -Z back
 */
inline Mesh makeCuboid(float w, float h, float d, RGB3 col) {
    Mesh m;
    float hw = w*0.5f, hh = h*0.5f, hd = d*0.5f;

    // 8 corners
    Vec3 lbf = {-hw,-hh, hd};  // left  bottom front
    Vec3 rbf = { hw,-hh, hd};  // right bottom front
    Vec3 rtf = { hw, hh, hd};  // right top    front
    Vec3 ltf = {-hw, hh, hd};  // left  top    front
    Vec3 lbb = {-hw,-hh,-hd};  // left  bottom back
    Vec3 rbb = { hw,-hh,-hd};  // right bottom back
    Vec3 rtb = { hw, hh,-hd};  // right top    back
    Vec3 ltb = {-hw, hh,-hd};  // left  top    back

    // Front  (+Z): CCW from outside = lbf,rbf,rtf,ltf
    addQuad(m, lbf, rbf, rtf, ltf, col);
    // Back   (-Z): CCW from outside = rbb,lbb,ltb,rtb
    addQuad(m, rbb, lbb, ltb, rtb, col);
    // Left   (-X): CCW from outside = lbb,lbf,ltf,ltb
    addQuad(m, lbb, lbf, ltf, ltb, col);
    // Right  (+X): CCW from outside = rbf,rbb,rtb,rtf
    addQuad(m, rbf, rbb, rtb, rtf, col);
    // Top    (+Y): CCW from outside = ltf,rtf,rtb,ltb
    addQuad(m, ltf, rtf, rtb, ltb, col);
    // Bottom (-Y): CCW from outside = lbb,rbb,rbf,lbf
    addQuad(m, lbb, rbb, rbf, lbf, col);

    m.upload();
    return m;
}

/*
 * Cuboid with per-face colours.
 * colours[6] = { front, back, left, right, top, bottom }
 */
inline Mesh makeCuboidColoured(float w, float h, float d,
                                RGB3 colours[6]) {
    Mesh m;
    float hw = w*0.5f, hh = h*0.5f, hd = d*0.5f;

    Vec3 lbf={-hw,-hh, hd}, rbf={ hw,-hh, hd};
    Vec3 rtf={ hw, hh, hd}, ltf={-hw, hh, hd};
    Vec3 lbb={-hw,-hh,-hd}, rbb={ hw,-hh,-hd};
    Vec3 rtb={ hw, hh,-hd}, ltb={-hw, hh,-hd};

    addQuad(m, lbf,rbf,rtf,ltf, colours[0]); // front
    addQuad(m, rbb,lbb,ltb,rtb, colours[1]); // back
    addQuad(m, lbb,lbf,ltf,ltb, colours[2]); // left
    addQuad(m, rbf,rbb,rtb,rtf, colours[3]); // right
    addQuad(m, ltf,rtf,rtb,ltb, colours[4]); // top
    addQuad(m, lbb,rbb,rbf,lbf, colours[5]); // bottom

    m.upload();
    return m;
}

// ---------------------------------------------------------------------------
// Triangular Prism
// ---------------------------------------------------------------------------
/*
 * A prism whose cross-section is an isoceles triangle in the XZ plane.
 * The triangle has:
 *   - base width w along X, centred at origin
 *   - depth  d along Z (apex at z = -d/2, base edge at z = +d/2... wait)
 *
 * Layout (looking down Y):
 *         apex (0, ?, -d/2)
 *          /\
 *         /  \
 *        /    \
 *       /______\
 *   (-w/2,?,+d/2)  (+w/2,?,+d/2)
 *
 * The prism extends h units along Y (bottom at y=0, top at y=h).
 *
 * Faces:
 *   - 2 triangular end caps (y=0 bottom, y=h top)
 *   - 3 rectangular side faces
 * Total: 2 + 3*2 = 8 triangles
 */
inline Mesh makeTriangularPrism(float w, float h, float d, RGB3 col) {
    Mesh m;
    float hw = w*0.5f, hd = d*0.5f;

    // Bottom triangle vertices (y=0)
    Vec3 b0 = { 0,   0, -hd};  // apex
    Vec3 b1 = {-hw,  0,  hd};  // base left
    Vec3 b2 = { hw,  0,  hd};  // base right

    // Top triangle vertices (y=h)
    Vec3 t0 = { 0,   h, -hd};
    Vec3 t1 = {-hw,  h,  hd};
    Vec3 t2 = { hw,  h,  hd};

    // Bottom cap (CCW from below = b0,b2,b1)
    m.addTriangle(b0, b2, b1, col);
    // Top cap (CCW from above = t0,t1,t2)
    m.addTriangle(t0, t1, t2, col);

    // Three side faces (quads)
    // Face 1: apex side (b0-t0 edge, b1-t1 edge)  left slanted face
    addQuad(m, b0, b1, t1, t0, col);
    // Face 2: right slanted face
    addQuad(m, b1, b2, t2, t1, col);
    // Face 3: back flat face (base of triangle)
    addQuad(m, b2, b0, t0, t2, col);

    m.upload();
    return m;
}

// ---------------------------------------------------------------------------
// Cylinder
// ---------------------------------------------------------------------------
/*
 * Cylinder with axis along Y.
 *   r    – radius
 *   h    – full height (base at y=0, top at y=h)
 *   segs – number of segments around the circumference (≥8 per spec)
 *
 * Faces:
 *   - Bottom disk: segs triangles (fan from centre)
 *   - Top disk:    segs triangles
 *   - Side:        segs quads = 2*segs triangles
 * Total: 4*segs triangles
 *
 * The spec requires cylinders for the axle (rotor hub) and the golf hole.
 */
inline Mesh makeCylinder(float r, float h, int segs, RGB3 col) {
    if (segs < 8) segs = 8; // enforce spec minimum
    Mesh m;

    float step = 2.0f * (float)M_PI / segs;
    Vec3 botCentre = {0, 0, 0};
    Vec3 topCentre = {0, h, 0};

    for (int i = 0; i < segs; i++) {
        float a0 = i       * step;
        float a1 = (i + 1) * step;

        // Points on bottom circle
        Vec3 b0 = {r*std::cos(a0), 0, r*std::sin(a0)};
        Vec3 b1 = {r*std::cos(a1), 0, r*std::sin(a1)};
        // Points on top circle
        Vec3 t0 = {r*std::cos(a0), h, r*std::sin(a0)};
        Vec3 t1 = {r*std::cos(a1), h, r*std::sin(a1)};

        // Bottom disk: CCW from below = centre, b1, b0
        m.addTriangle(botCentre, b1, b0, col);
        // Top disk:    CCW from above = centre, t0, t1
        m.addTriangle(topCentre, t0, t1, col);
        // Side quad:   outward-facing CCW = b0, b1, t1, t0
        addQuad(m, b0, b1, t1, t0, col);
    }

    m.upload();
    return m;
}

// ---------------------------------------------------------------------------
// Cone
// ---------------------------------------------------------------------------
/*
 * Cone with axis along Y.
 *   r    – base radius
 *   h    – height (base at y=0, apex at y=h)
 *   segs – segments around circumference (≥8 per spec)
 *
 * Faces:
 *   - Base disk: segs triangles (fan from centre)
 *   - Side:      segs triangles (from each base edge to apex)
 * Total: 2*segs triangles
 */
inline Mesh makeCone(float r, float h, int segs, RGB3 col) {
    if (segs < 8) segs = 8;
    Mesh m;

    float step = 2.0f * (float)M_PI / segs;
    Vec3 apex      = {0, h, 0};
    Vec3 baseCentre = {0, 0, 0};

    for (int i = 0; i < segs; i++) {
        float a0 = i       * step;
        float a1 = (i + 1) * step;

        Vec3 b0 = {r*std::cos(a0), 0, r*std::sin(a0)};
        Vec3 b1 = {r*std::cos(a1), 0, r*std::sin(a1)};

        // Base disk: CCW from below = centre, b1, b0
        m.addTriangle(baseCentre, b1, b0, col);
        // Side: CCW from outside = b0, b1, apex
        m.addTriangle(b0, b1, apex, col);
    }

    m.upload();
    return m;
}

// ---------------------------------------------------------------------------
// Blade (flat elongated cuboid, used for windmill blades)
// ---------------------------------------------------------------------------
/*
 * A thin, flat rectangular blade.
 *   length – along Y axis
 *   width  – along X axis
 *   thick  – along Z axis (small, makes it look like a flat plank)
 *
 * Origin at the ROOT (bottom centre) of the blade so rotation about
 * the rotor axle (Z axis through root) works correctly.
 * Blade extends from y=0 (root) to y=length (tip).
 */
inline Mesh makeBlade(float length, float width, float thick, RGB3 col) {
    Mesh m;
    float hw = width*0.5f, ht = thick*0.5f;

    Vec3 lbf = {-hw, 0,       ht};
    Vec3 rbf = { hw, 0,       ht};
    Vec3 rtf = { hw, length,  ht};
    Vec3 ltf = {-hw, length,  ht};
    Vec3 lbb = {-hw, 0,      -ht};
    Vec3 rbb = { hw, 0,      -ht};
    Vec3 rtb = { hw, length, -ht};
    Vec3 ltb = {-hw, length, -ht};

    addQuad(m, lbf,rbf,rtf,ltf, col); // front
    addQuad(m, rbb,lbb,ltb,rtb, col); // back
    addQuad(m, lbb,lbf,ltf,ltb, col); // left
    addQuad(m, rbf,rbb,rtb,rtf, col); // right
    addQuad(m, ltf,rtf,rtb,ltb, col); // top
    addQuad(m, lbb,rbb,rbf,lbf, col); // bottom

    m.upload();
    return m;
}

#endif // SHAPES3D_HPP
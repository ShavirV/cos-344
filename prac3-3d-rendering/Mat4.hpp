#ifndef MAT4_HPP
#define MAT4_HPP

/*
 * mat4.hpp  –  Custom 4x4 matrix + vec3 math for 3D rendering
 * =============================================================
 * Spec forbids GLM math functions. All MVP construction lives here.
 *
 * Convention: COLUMN-MAJOR storage, matching OpenGL.
 *   m[col][row]  →  m[0] is the first column.
 *
 * This means the translation column is m[3][0..2], which is the
 * standard OpenGL / GLSL layout so we can pass &m[0][0] directly
 * to glUniformMatrix4fv(..., GL_FALSE, ...).
 *
 * Structs:
 *   Vec3  – 3-component float vector
 *   Vec4  – 4-component float vector  
 *   Mat4  – 4x4 column-major matrix
 *
 * Factory functions:
 *   mat4::identity()
 *   mat4::translate(x,y,z)
 *   mat4::scale(x,y,z)
 *   mat4::rotateX/Y/Z(radians)
 *   mat4::perspective(fovY_rad, aspect, near, far)
 *   mat4::lookAt(eye, target, up)
 */

#include <cmath>

// ---------------------------------------------------------------------------
// Vec3
// ---------------------------------------------------------------------------
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s)       const { return {x*s,   y*s,   z*s  }; }
    float dot(const Vec3& o)      const { return x*o.x + y*o.y + z*o.z; }

    Vec3 cross(const Vec3& o) const {
        return { y*o.z - z*o.y,
                 z*o.x - x*o.z,
                 x*o.y - y*o.x };
    }

    float length() const { return std::sqrt(x*x + y*y + z*z); }

    Vec3 normalise() const {
        float l = length();
        if (l < 1e-6f) return {0,0,0};
        return {x/l, y/l, z/l};
    }
};

// ---------------------------------------------------------------------------
// Vec4
// ---------------------------------------------------------------------------
struct Vec4 {
    float x, y, z, w;
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vec4(Vec3 v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
};

// ---------------------------------------------------------------------------
// Mat4  –  column-major 4x4
// ---------------------------------------------------------------------------
struct Mat4 {
    float m[4][4]; // m[col][row]

    // Default: identity
    Mat4() {
        for (int c = 0; c < 4; c++)
            for (int r = 0; r < 4; r++)
                m[c][r] = (c == r) ? 1.0f : 0.0f;
    }

    // Matrix * Matrix
    Mat4 operator*(const Mat4& o) const {
        Mat4 res;
        for (int c = 0; c < 4; c++)
            for (int r = 0; r < 4; r++) {
                res.m[c][r] = 0.0f;
                for (int k = 0; k < 4; k++)
                    res.m[c][r] += m[k][r] * o.m[c][k];
            }
        return res;
    }

    // Matrix * Vec4
    Vec4 operator*(const Vec4& v) const {
        return {
            m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z + m[3][0]*v.w,
            m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z + m[3][1]*v.w,
            m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z + m[3][2]*v.w,
            m[0][3]*v.x + m[1][3]*v.y + m[2][3]*v.z + m[3][3]*v.w
        };
    }

    // Pointer to first element for glUniformMatrix4fv
    const float* ptr() const { return &m[0][0]; }
};

// ---------------------------------------------------------------------------
// Factory functions
// ---------------------------------------------------------------------------
namespace mat4 {

inline Mat4 identity() { return Mat4(); }

// Translation
inline Mat4 translate(float tx, float ty, float tz) {
    Mat4 t;
    t.m[3][0] = tx;  // col 3, row 0
    t.m[3][1] = ty;
    t.m[3][2] = tz;
    return t;
}

// Non-uniform scale
inline Mat4 scale(float sx, float sy, float sz) {
    Mat4 s;
    s.m[0][0] = sx;
    s.m[1][1] = sy;
    s.m[2][2] = sz;
    return s;
}

// Rotation about X axis (pitch), angle in radians
inline Mat4 rotateX(float a) {
    Mat4 r;
    float c = std::cos(a), s = std::sin(a);
    r.m[1][1] =  c;  r.m[2][1] = -s;
    r.m[1][2] =  s;  r.m[2][2] =  c;
    return r;
}

// Rotation about Y axis (yaw), angle in radians
inline Mat4 rotateY(float a) {
    Mat4 r;
    float c = std::cos(a), s = std::sin(a);
    r.m[0][0] =  c;  r.m[2][0] =  s;
    r.m[0][2] = -s;  r.m[2][2] =  c;
    return r;
}

// Rotation about Z axis (roll), angle in radians
inline Mat4 rotateZ(float a) {
    Mat4 r;
    float c = std::cos(a), s = std::sin(a);
    r.m[0][0] =  c;  r.m[1][0] = -s;
    r.m[0][1] =  s;  r.m[1][1] =  c;
    return r;
}

/*
 * Rotation about an arbitrary axis (unit vector).
 * Uses Rodrigues' rotation formula.
 * axis must be normalised before calling.
 */
inline Mat4 rotateAxis(Vec3 axis, float angle) {
    float c = std::cos(angle);
    float s = std::sin(angle);
    float t = 1.0f - c;
    float x = axis.x, y = axis.y, z = axis.z;

    Mat4 r;
    // Column 0
    r.m[0][0] = t*x*x + c;
    r.m[0][1] = t*x*y + s*z;
    r.m[0][2] = t*x*z - s*y;
    r.m[0][3] = 0;
    // Column 1
    r.m[1][0] = t*x*y - s*z;
    r.m[1][1] = t*y*y + c;
    r.m[1][2] = t*y*z + s*x;
    r.m[1][3] = 0;
    // Column 2
    r.m[2][0] = t*x*z + s*y;
    r.m[2][1] = t*y*z - s*x;
    r.m[2][2] = t*z*z + c;
    r.m[2][3] = 0;
    // Column 3
    r.m[3][0] = r.m[3][1] = r.m[3][2] = 0;
    r.m[3][3] = 1;
    return r;
}

/*
 * Perspective projection matrix.
 * fovY   – vertical field of view in RADIANS
 * aspect – width / height
 * near   – near clip plane (positive)
 * far    – far  clip plane (positive)
 *
 * Maps the view frustum to NDC [-1,1]^3.
 */
inline Mat4 perspective(float fovY, float aspect, float nearZ, float farZ) {
    float f = 1.0f / std::tan(fovY * 0.5f); // cot(fovY/2)
    float nf = 1.0f / (nearZ - farZ);

    Mat4 p;
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            p.m[c][r] = 0.0f;

    p.m[0][0] = f / aspect;
    p.m[1][1] = f;
    p.m[2][2] = (farZ + nearZ) * nf;       // -(far+near)/(far-near)
    p.m[2][3] = -1.0f;                      // perspective divide trigger
    p.m[3][2] = 2.0f * farZ * nearZ * nf;  // -2*far*near/(far-near)
    return p;
}

/*
 * View matrix (lookAt).
 * eye    – camera position
 * target – point camera is looking at
 * up     – world up vector (usually {0,1,0})
 *
 * Constructs an orthonormal camera basis and builds the view matrix
 * that transforms world coordinates into camera space.
 */
inline Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 f = (target - eye).normalise();  // forward
    Vec3 r = f.cross(up).normalise();     // right
    Vec3 u = r.cross(f);                  // true up

    Mat4 v;
    // Row vectors of the rotation part
    v.m[0][0] =  r.x;  v.m[1][0] =  r.y;  v.m[2][0] =  r.z;
    v.m[0][1] =  u.x;  v.m[1][1] =  u.y;  v.m[2][1] =  u.z;
    v.m[0][2] = -f.x;  v.m[1][2] = -f.y;  v.m[2][2] = -f.z;
    // Translation: dot of each basis vector with -eye
    v.m[3][0] = -r.dot(eye);
    v.m[3][1] = -u.dot(eye);
    v.m[3][2] =  f.dot(eye);
    v.m[3][3] =  1.0f;
    return v;
}

} // namespace mat4

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#endif // MAT4_HPP
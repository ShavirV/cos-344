#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <array>

/*
* Vertex format: [x,y,z, nx,ny,nz, u,v] = 8 floats
*   0 = position (vec3)
*   1 = normal   (vec3)
*   2 = UV       (vec2)
*
* SPHERE
* same as prac 3, lat long tesselation 
* latSegs = latitude bands (min 4)
* lonSegs = longitude slices (min 4)
* UV: u = lon/(2pi), v = lat/pi  -> unwrap for dimples
* Normals: outward unit radial direction
* Displacement: optionally sampled from TextureGen::sampleDisplacement(u,v)
*   and applied along the normal 
*
* PLANE
* Grid of (divs × divs) quads in the XZ plane.
* Normal = (0,1,0) for all vertices.
* UV: tiled n times across the plane for texture repetition.
* divs = number of quad subdivisions per axis
*   Higher divs, more vertices, better lighting gradient on floor.
*
* wireframe uses GL_LINESs
 */

#include <vector>
#include <cmath>
#include <algorithm>
#include <GL/glew.h>
#include "TextureGen.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//push a vertex into a float vector
static inline void pushVert(std::vector<float>& v,
    float x, float y, float z,
    float nx, float ny, float nz,
    float u,  float vv)
{
    v.push_back(x);  v.push_back(y);  v.push_back(z);
    v.push_back(nx); v.push_back(ny); v.push_back(nz);
    v.push_back(u);  v.push_back(vv);
}

class Sphere {
public:
    GLuint vao=0, filledVBO=0, wireVBO=0;
    int filledCount=0, wireCount=0;
    int latSegs=16, lonSegs=16;
    float radius=1.0f;
    bool displaced=false;

    void init() {
        glGenVertexArrays(1,&vao);
        glGenBuffers(1,&filledVBO);
        glGenBuffers(1,&wireVBO);
    }

    void rebuild(bool applyDisplacement) {
        displaced = applyDisplacement;
        std::vector<float> filled, wire;

        int ls = std::max(4,latSegs);
        int os = std::max(4,lonSegs);

        //We build a tri list so displacement is per-vertex
        for (int lat=0; lat<ls; lat++) {
            float theta0 = (float)M_PI *  lat / ls;
            float theta1 = (float)M_PI * (lat + 1) / ls;

            for (int lon=0; lon<os; lon++) {
                float phi0 = 2.0f*(float)M_PI *  lon / os;
                float phi1 = 2.0f*(float)M_PI * (lon + 1) / os;

                // UV for each corner
                float u0 = (float)lon / os;
                float u1 = (float)(lon + 1) / os;
                float v0 = (float)lat / ls;
                float v1 = (float)(lat + 1)  / ls;

                // Normals (unit sphere, normal = position on unit sphere)
                auto norm = [](float th, float ph) -> std::array<float,3> {
                    return {sinf(th)*cosf(ph), cosf(th), sinf(th)*sinf(ph)};
                };

                auto n00 = norm(theta0, phi0);
                auto n01 = norm(theta0, phi1);
                auto n10 = norm(theta1, phi0);
                auto n11 = norm(theta1, phi1);

                //position = radius * normal, then displace
                auto makeVert = [&](std::array<float,3> n, float u, float vv) -> std::array<float,8>
                {
                    float r = radius;
                    if (applyDisplacement) {
                        float dispVal = TextureGen::sampleDisplacement(u, vv);
                        r = radius * dispVal;  //dispVal in [0.2..1.0]
                    }
                    return {n[0]*r, n[1]*r, n[2]*r,
                            n[0],   n[1],   n[2],
                            u, vv};
                };

                auto v00 = makeVert(n00, u0, v0);
                auto v01 = makeVert(n01, u1, v0);
                auto v10 = makeVert(n10, u0, v1);
                auto v11 = makeVert(n11, u1, v1);

                //t1: v00, v10, v11
                //t2: v00, v11, v01
                auto push = [&](std::array<float,8>& vd) {
                    for (float f : vd) filled.push_back(f);
                };

                push(v00); push(v10); push(v11);
                push(v00); push(v11); push(v01);

                //wire edges, only add edge if in lat long ordering 
                auto pushW = [&](std::array<float,8>& a, std::array<float,8>& b) {
                    for (float f : a) wire.push_back(f);
                    for (float f : b) wire.push_back(f);
                };
                pushW(v00, v10);   // vertical edge
                pushW(v00, v01);   // horizontal edge
                if (lat == ls-1) pushW(v10, v11);  // bottom edge
                if (lon == os-1) pushW(v01, v11);  // right edge
            }
        }

        filledCount = (int)filled.size() / 8;
        wireCount   = (int)wire.size()   / 8;

        auto upload = [](GLuint vbo, std::vector<float>& data) {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(float),
                         data.data(), GL_DYNAMIC_DRAW);
        };

        glBindVertexArray(vao);
        upload(filledVBO, filled);
        setAttribs();
        glBindVertexArray(0);

        glBindVertexArray(vao); //reuse vao for wire (we'll bind vbo manually in draw)
        glBindVertexArray(0);

        //wire data stored in a separate buffer  
        glBindBuffer(GL_ARRAY_BUFFER, wireVBO);
        glBufferData(GL_ARRAY_BUFFER, wire.size()*sizeof(float),
                     wire.data(), GL_DYNAMIC_DRAW);
    }

    void draw(bool wf) {
        glBindVertexArray(vao);
        if (!wf) {
            glBindBuffer(GL_ARRAY_BUFFER, filledVBO);
            setAttribs();
            glDrawArrays(GL_TRIANGLES, 0, filledCount);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, wireVBO);
            setAttribs();
            glDrawArrays(GL_LINES, 0, wireCount);
        }
        glBindVertexArray(0);
    }

    ~Sphere() {
        if (filledVBO) glDeleteBuffers(1,&filledVBO);
        if (wireVBO)   glDeleteBuffers(1,&wireVBO);
        if (vao)       glDeleteVertexArrays(1,&vao);
    }

private:
    static void setAttribs() {
        int stride = 8*sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6*sizeof(float)));
        glEnableVertexAttribArray(2);
    }
};

//Plane (tessellated grid)
class Plane {
public:
    GLuint vao=0, filledVBO=0, wireVBO=0;
    int filledCount=0, wireCount=0;
    int divs=8;           // number of quads per axis
    float size=6.0f;      // full extent (from -size/2 to +size/2)
    float uvTile=4.0f;    // how many times texture tiles across

    void init() {
        glGenVertexArrays(1,&vao);
        glGenBuffers(1,&filledVBO);
        glGenBuffers(1,&wireVBO);
    }

    void rebuild() {
        std::vector<float> filled, wire;

        int d = std::max(1, divs);
        float step = size / d;
        float half = size * 0.5f;

        for (int row=0; row<d; row++) {
            for (int col=0; col<d; col++) {
                float x0 = -half + col * step;
                float x1 = -half + (col+1) * step;
                float z0 = -half + row * step;
                float z1 = -half + (row+1) * step;

                float u0 = (float)col / d * uvTile;
                float u1 = (float)(col+1) / d * uvTile;
                float v0 = (float)row / d * uvTile;
                float v1 = (float)(row+1) / d * uvTile;

                //Y=0, normal upward
                auto p = [&](float x,float z,float u,float vv) {
                    pushVert(filled, x,0,z, 0,1,0, u,vv);
                };
                //Triangle 1
                p(x0,z0,u0,v0); p(x1,z0,u1,v0); p(x1,z1,u1,v1);
                //Triangle 2
                p(x0,z0,u0,v0); p(x1,z1,u1,v1); p(x0,z1,u0,v1);

                //Wire edges
                auto pw = [&](float xa,float za,float ua,float va,
                               float xb,float zb,float ub,float vb) {
                    pushVert(wire,xa,0,za,0,1,0,ua,va);
                    pushVert(wire,xb,0,zb,0,1,0,ub,vb);
                };
                pw(x0,z0,u0,v0, x1,z0,u1,v0);
                pw(x0,z0,u0,v0, x0,z1,u0,v1);
                if (row==d-1) pw(x0,z1,u0,v1, x1,z1,u1,v1);
                if (col==d-1) pw(x1,z0,u1,v0, x1,z1,u1,v1);
            }
        }

        filledCount = (int)filled.size() / 8;
        wireCount   = (int)wire.size()   / 8;

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, filledVBO);
        glBufferData(GL_ARRAY_BUFFER, filled.size()*sizeof(float), filled.data(), GL_DYNAMIC_DRAW);
        setAttribs();
        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, wireVBO);
        glBufferData(GL_ARRAY_BUFFER, wire.size()*sizeof(float), wire.data(), GL_DYNAMIC_DRAW);
    }

    void draw(bool wf) {
        glBindVertexArray(vao);
        if (!wf) {
            glBindBuffer(GL_ARRAY_BUFFER, filledVBO);
            setAttribs();
            glDrawArrays(GL_TRIANGLES, 0, filledCount);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, wireVBO);
            setAttribs();
            glDrawArrays(GL_LINES, 0, wireCount);
        }
        glBindVertexArray(0);
    }

    ~Plane() {
        if (filledVBO) glDeleteBuffers(1,&filledVBO);
        if (wireVBO)   glDeleteBuffers(1,&wireVBO);
        if (vao)       glDeleteVertexArrays(1,&vao);
    }

private:
    static void setAttribs() {
        int stride = 8*sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6*sizeof(float)));
        glEnableVertexAttribArray(2);
    }
};

#endif // GEOMETRY_HPP
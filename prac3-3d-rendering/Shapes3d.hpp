#ifndef SHAPES3D_HPP
#define SHAPES3D_HPP

#include "Mesh.hpp"
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//helpers
static void addQuad(Mesh& m,
                    Vec3 v0, Vec3 v1, Vec3 v2, Vec3 v3, RGB3 col) {
    m.addTriangle(v0, v1, v2, col);
    m.addTriangle(v0, v2, v3, col);
}

inline Mesh makeCuboid(float w, float h, float d, RGB3 col) {
    Mesh m;
    float hw=w*.5f, hh=h*.5f, hd=d*.5f;
    Vec3 lbf={-hw,-hh, hd}, rbf={ hw,-hh, hd};
    Vec3 rtf={ hw, hh, hd}, ltf={-hw, hh, hd};
    Vec3 lbb={-hw,-hh,-hd}, rbb={ hw,-hh,-hd};
    Vec3 rtb={ hw, hh,-hd}, ltb={-hw, hh,-hd};
    addQuad(m,lbf,rbf,rtf,ltf,col); // front
    addQuad(m,rbb,lbb,ltb,rtb,col); // back
    addQuad(m,lbb,lbf,ltf,ltb,col); // left
    addQuad(m,rbf,rbb,rtb,rtf,col); // right
    addQuad(m,ltf,rtf,rtb,ltb,col); // top
    addQuad(m,lbb,rbb,rbf,lbf,col); // bottom
    m.upload(); return m;
}

//per-face colour cuboid (front,back,left,right,top,bottom)
inline Mesh makeCuboidColoured(float w, float h, float d, RGB3 c[6]) {
    Mesh m;
    float hw=w*.5f, hh=h*.5f, hd=d*.5f;
    Vec3 lbf={-hw,-hh, hd}, rbf={ hw,-hh, hd};
    Vec3 rtf={ hw, hh, hd}, ltf={-hw, hh, hd};
    Vec3 lbb={-hw,-hh,-hd}, rbb={ hw,-hh,-hd};
    Vec3 rtb={ hw, hh,-hd}, ltb={-hw, hh,-hd};
    addQuad(m,lbf,rbf,rtf,ltf,c[0]);
    addQuad(m,rbb,lbb,ltb,rtb,c[1]);
    addQuad(m,lbb,lbf,ltf,ltb,c[2]);
    addQuad(m,rbf,rbb,rtb,rtf,c[3]);
    addQuad(m,ltf,rtf,rtb,ltb,c[4]);
    addQuad(m,lbb,rbb,rbf,lbf,c[5]);
    m.upload(); return m;
}


inline Mesh makeTriangularPrism(float w, float h, float d, RGB3 col) {
    Mesh m;
    float hw=w*.5f, hd=d*.5f;
    Vec3 b0={0,0,-hd}, b1={-hw,0,hd}, b2={hw,0,hd};
    Vec3 t0={0,h,-hd}, t1={-hw,h,hd}, t2={hw,h,hd};
    m.addTriangle(b0,b2,b1,col);     // bottom cap
    m.addTriangle(t0,t1,t2,col);     // top cap
    addQuad(m,b0,b1,t1,t0,col);      // left face
    addQuad(m,b1,b2,t2,t1,col);      // front face
    addQuad(m,b2,b0,t0,t2,col);      // right face
    m.upload(); return m;
}

inline Mesh makeCylinder(float r, float h, int segs, RGB3 col) {
    if (segs<8) segs=8;
    Mesh m;
    float step=2.f*(float)M_PI/segs;
    Vec3 bc={0,0,0}, tc={0,h,0};
    for (int i=0;i<segs;i++){
        float a0=i*step, a1=(i+1)*step;
        Vec3 b0={r*cosf(a0),0,r*sinf(a0)};
        Vec3 b1={r*cosf(a1),0,r*sinf(a1)};
        Vec3 t0={r*cosf(a0),h,r*sinf(a0)};
        Vec3 t1={r*cosf(a1),h,r*sinf(a1)};
        m.addTriangle(bc,b1,b0,col);   // bottom
        m.addTriangle(tc,t0,t1,col);   // top
        addQuad(m,b0,b1,t1,t0,col);    // side
    }
    m.upload(); return m;
}

// Two-colour cylinder (body col + cap col, e.g. metallic axle with darker ends)
inline Mesh makeCylinderColoured(float r, float h, int segs,
                                  RGB3 bodyCol, RGB3 capCol) {
    if (segs<8) segs=8;
    Mesh m;
    float step=2.f*(float)M_PI/segs;
    Vec3 bc={0,0,0}, tc={0,h,0};
    for (int i=0;i<segs;i++){
        float a0=i*step, a1=(i+1)*step;
        Vec3 b0={r*cosf(a0),0,r*sinf(a0)};
        Vec3 b1={r*cosf(a1),0,r*sinf(a1)};
        Vec3 t0={r*cosf(a0),h,r*sinf(a0)};
        Vec3 t1={r*cosf(a1),h,r*sinf(a1)};
        m.addTriangle(bc,b1,b0,capCol);
        m.addTriangle(tc,t0,t1,capCol);
        addQuad(m,b0,b1,t1,t0,bodyCol);
    }
    m.upload(); return m;
}


inline Mesh makeCone(float r, float h, int segs, RGB3 col) {
    if (segs<8) segs=8;
    Mesh m;
    float step=2.f*(float)M_PI/segs;
    Vec3 apex={0,h,0}, bc={0,0,0};
    for (int i=0;i<segs;i++){
        float a0=i*step, a1=(i+1)*step;
        Vec3 b0={r*cosf(a0),0,r*sinf(a0)};
        Vec3 b1={r*cosf(a1),0,r*sinf(a1)};
        m.addTriangle(bc,b1,b0,col);    // base
        m.addTriangle(b0,b1,apex,col);  // side
    }
    m.upload(); return m;
}

//funny truncated cone thing that the windmill is made out of
inline Mesh makeFrustum(float rBot, float rTop, float h, int segs, RGB3 col) {
    if (segs<8) segs=8;
    Mesh m;
    float step=2.f*(float)M_PI/segs;
    Vec3 bc={0,0,0}, tc={0,h,0};
    for (int i=0;i<segs;i++){
        float a0=i*step, a1=(i+1)*step;
        Vec3 b0={rBot*cosf(a0), 0, rBot*sinf(a0)};
        Vec3 b1={rBot*cosf(a1), 0, rBot*sinf(a1)};
        Vec3 t0={rTop*cosf(a0), h, rTop*sinf(a0)};
        Vec3 t1={rTop*cosf(a1), h, rTop*sinf(a1)};
        m.addTriangle(bc,b1,b0,col);    // bottom cap
        m.addTriangle(tc,t0,t1,col);    // top cap
        addQuad(m,b0,b1,t1,t0,col);     // side
    }
    m.upload(); return m;
}

//two-colour frustum side face vs cap colours
inline Mesh makeFrustumColoured(float rBot, float rTop, float h, int segs,
                                 RGB3 sideCol, RGB3 capCol) {
    if (segs<8) segs=8;
    Mesh m;
    float step=2.f*(float)M_PI/segs;
    Vec3 bc={0,0,0}, tc={0,h,0};
    for (int i=0;i<segs;i++){
        float a0=i*step, a1=(i+1)*step;
        Vec3 b0={rBot*cosf(a0),0,rBot*sinf(a0)};
        Vec3 b1={rBot*cosf(a1),0,rBot*sinf(a1)};
        Vec3 t0={rTop*cosf(a0),h,rTop*sinf(a0)};
        Vec3 t1={rTop*cosf(a1),h,rTop*sinf(a1)};
        m.addTriangle(bc,b1,b0,capCol);
        m.addTriangle(tc,t0,t1,capCol);
        addQuad(m,b0,b1,t1,t0,sideCol);
    }
    m.upload(); return m;
}

//sphere using lat lon tesselation
inline Mesh makeSphere(float r, int latSegs, int lonSegs, RGB3 col) {
    if (latSegs<4) latSegs=4;
    if (lonSegs<4) lonSegs=4;
    Mesh m;

    // latSegs bands between poles → latSegs+1 rings of vertices
    for (int lat=0; lat<latSegs; lat++) {
        float theta0 = (float)M_PI * lat / latSegs;  // 0..PI
        float theta1 = (float)M_PI * (lat+1) / latSegs;

        float y0 = r * cosf(theta0);
        float y1 = r * cosf(theta1);
        float r0 = r * sinf(theta0);  //ring radius at lat band bottom
        float r1 = r * sinf(theta1);  //ring radius at lat band top

        for (int lon=0; lon<lonSegs; lon++) {
            float phi0 = 2.f*(float)M_PI * lon       / lonSegs;
            float phi1 = 2.f*(float)M_PI * (lon+1)   / lonSegs;

            Vec3 v00 = {r0*cosf(phi0), y0, r0*sinf(phi0)};
            Vec3 v01 = {r0*cosf(phi1), y0, r0*sinf(phi1)};
            Vec3 v10 = {r1*cosf(phi0), y1, r1*sinf(phi0)};
            Vec3 v11 = {r1*cosf(phi1), y1, r1*sinf(phi1)};

            //at poles one edge degenerates to a point – use triangle
            if (lat == 0) {
                //north pole, v00 and v01 converge
                m.addTriangle(v00, v01, v10, col);
                m.addTriangle(v01, v11, v10, col);
            } else if (lat == latSegs-1) {
                //south pole
                m.addTriangle(v00, v01, v10, col);
                m.addTriangle(v01, v11, v10, col);
            } else {
                addQuad(m, v00, v01, v11, v10, col);
            }
        }
    }
    m.upload(); return m;
}

inline Mesh makeBlade(float length, float width, float thick, RGB3 col) {
    Mesh m;
    float hw=width*.5f, ht=thick*.5f;
    Vec3 lbf={-hw,0,     ht}, rbf={ hw,0,     ht};
    Vec3 rtf={ hw,length,ht}, ltf={-hw,length,ht};
    Vec3 lbb={-hw,0,    -ht}, rbb={ hw,0,    -ht};
    Vec3 rtb={ hw,length,-ht},ltb={-hw,length,-ht};
    addQuad(m,lbf,rbf,rtf,ltf,col);
    addQuad(m,rbb,lbb,ltb,rtb,col);
    addQuad(m,lbb,lbf,ltf,ltb,col);
    addQuad(m,rbf,rbb,rtb,rtf,col);
    addQuad(m,ltf,rtf,rtb,ltb,col);
    addQuad(m,lbb,rbb,rbf,lbf,col);
    m.upload(); return m;
}

#endif // SHAPES3D_HPP
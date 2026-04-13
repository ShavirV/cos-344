#ifndef MESH_HPP
#define MESH_HPP

#include <vector>
#include <GL/glew.h>
#include "Mat4.hpp"

struct RGB3 { float r, g, b; };

class Mesh {
public:
    Mesh() : vao(0), filledVBO(0), wireVBO(0),
             filledCount(0), wireCount(0) {}

    ~Mesh() { free(); }

    //Disable copy (owns GPU resources)
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    //Move constructor so we can store Meshes in vectors
    Mesh(Mesh&& o) noexcept
        : vao(o.vao), filledVBO(o.filledVBO), wireVBO(o.wireVBO),
          filledCount(o.filledCount), wireCount(o.wireCount),
          filledData(std::move(o.filledData)),
          wireData(std::move(o.wireData))
    {
        o.vao = o.filledVBO = o.wireVBO = 0;
    }
    void addTriangle(Vec3 a, Vec3 b, Vec3 c, RGB3 col) {
        pushVert(filledData, a, col);
        pushVert(filledData, b, col);
        pushVert(filledData, c, col);

        // Wireframe edges: each edge appears as a GL_LINES pair
        pushVert(wireData, a, col); pushVert(wireData, b, col);
        pushVert(wireData, b, col); pushVert(wireData, c, col);
        pushVert(wireData, c, col); pushVert(wireData, a, col);
    }

    void upload() {
        free(); // delete existing GPU objects if any

        filledCount = (int)filledData.size() / 6;
        wireCount   = (int)wireData.size()   / 6;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Filled VBO
        glGenBuffers(1, &filledVBO);
        glBindBuffer(GL_ARRAY_BUFFER, filledVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     filledData.size() * sizeof(float),
                     filledData.data(), GL_STATIC_DRAW);
        setupAttribs();

        // Wire VBO
        glGenBuffers(1, &wireVBO);
        glBindBuffer(GL_ARRAY_BUFFER, wireVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     wireData.size() * sizeof(float),
                     wireData.data(), GL_STATIC_DRAW);
        setupAttribs();

        glBindVertexArray(0);

        // CPU data no longer needed after upload
        filledData.clear(); filledData.shrink_to_fit();
        wireData.clear();   wireData.shrink_to_fit();
    }

    void draw(bool wireframe) const {
        if (!vao) return;
        glBindVertexArray(vao);

        if (!wireframe) {
            glBindBuffer(GL_ARRAY_BUFFER, filledVBO);
            setupAttribsConst();
            glDrawArrays(GL_TRIANGLES, 0, filledCount);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, wireVBO);
            setupAttribsConst();
            glDrawArrays(GL_LINES, 0, wireCount);
        }

        glBindVertexArray(0);
    }

private:
    GLuint vao, filledVBO, wireVBO;
    int filledCount, wireCount;
    std::vector<float> filledData, wireData;

    static void pushVert(std::vector<float>& buf, Vec3 p, RGB3 c) {
        buf.push_back(p.x); buf.push_back(p.y); buf.push_back(p.z);
        buf.push_back(c.r); buf.push_back(c.g); buf.push_back(c.b);
    }

    // Set vertex attrib pointers for [x,y,z, r,g,b] layout
    static void setupAttribs() {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              6*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              6*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    void setupAttribsConst() const { setupAttribs(); }

    void free() {
        if (filledVBO) { glDeleteBuffers(1, &filledVBO); filledVBO = 0; }
        if (wireVBO)   { glDeleteBuffers(1, &wireVBO);   wireVBO   = 0; }
        if (vao)       { glDeleteVertexArrays(1, &vao);  vao       = 0; }
    }
};

#endif // MESH_HPP
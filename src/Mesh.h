#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <cmath>

// Simple mesh container
struct Mesh {
    unsigned int VAO, VBO;
    int vertCount;

    void draw() const {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertCount);
        glBindVertexArray(0);
    }
};

// Upload interleaved pos+normal data
static Mesh uploadMesh(const std::vector<float>& data) {
    Mesh m;
    m.vertCount = (int)data.size() / 6;
    glGenVertexArrays(1, &m.VAO);
    glGenBuffers(1, &m.VBO);
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return m;
}

// ---- Box ----
static Mesh makeCube() {
    // pos(3) + normal(3) per vertex, 36 vertices
    std::vector<float> v = {
        // Back
        -0.5f,-0.5f,-0.5f, 0,0,-1,  0.5f,-0.5f,-0.5f, 0,0,-1,  0.5f, 0.5f,-0.5f, 0,0,-1,
         0.5f, 0.5f,-0.5f, 0,0,-1, -0.5f, 0.5f,-0.5f, 0,0,-1, -0.5f,-0.5f,-0.5f, 0,0,-1,
        // Front
        -0.5f,-0.5f, 0.5f, 0,0,1,   0.5f,-0.5f, 0.5f, 0,0,1,   0.5f, 0.5f, 0.5f, 0,0,1,
         0.5f, 0.5f, 0.5f, 0,0,1,  -0.5f, 0.5f, 0.5f, 0,0,1,  -0.5f,-0.5f, 0.5f, 0,0,1,
        // Left
        -0.5f, 0.5f, 0.5f,-1,0,0,  -0.5f, 0.5f,-0.5f,-1,0,0,  -0.5f,-0.5f,-0.5f,-1,0,0,
        -0.5f,-0.5f,-0.5f,-1,0,0,  -0.5f,-0.5f, 0.5f,-1,0,0,  -0.5f, 0.5f, 0.5f,-1,0,0,
        // Right
         0.5f, 0.5f, 0.5f, 1,0,0,   0.5f, 0.5f,-0.5f, 1,0,0,   0.5f,-0.5f,-0.5f, 1,0,0,
         0.5f,-0.5f,-0.5f, 1,0,0,   0.5f,-0.5f, 0.5f, 1,0,0,   0.5f, 0.5f, 0.5f, 1,0,0,
        // Bottom
        -0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f, 0.5f, 0,-1,0,
         0.5f,-0.5f, 0.5f, 0,-1,0, -0.5f,-0.5f, 0.5f, 0,-1,0, -0.5f,-0.5f,-0.5f, 0,-1,0,
        // Top
        -0.5f, 0.5f,-0.5f, 0,1,0,   0.5f, 0.5f,-0.5f, 0,1,0,   0.5f, 0.5f, 0.5f, 0,1,0,
         0.5f, 0.5f, 0.5f, 0,1,0,  -0.5f, 0.5f, 0.5f, 0,1,0,  -0.5f, 0.5f,-0.5f, 0,1,0,
    };
    return uploadMesh(v);
}

// ---- Cylinder (stack=1, sectors=12) ----
static Mesh makeCylinder(float rTop=0.5f, float rBot=0.5f, float h=1.0f, int sec=12) {
    std::vector<float> v;
    const float PI = 3.14159265f;
    auto push = [&](float x, float y, float z, float nx, float ny, float nz){
        v.insert(v.end(),{x,y,z,nx,ny,nz});
    };
    // sides
    for(int i=0;i<sec;i++){
        float a0=2*PI*i/sec, a1=2*PI*(i+1)/sec;
        float x0t=rTop*cosf(a0), z0t=rTop*sinf(a0);
        float x1t=rTop*cosf(a1), z1t=rTop*sinf(a1);
        float x0b=rBot*cosf(a0), z0b=rBot*sinf(a0);
        float x1b=rBot*cosf(a1), z1b=rBot*sinf(a1);
        float nx0=cosf(a0), nz0=sinf(a0), nx1=cosf(a1), nz1=sinf(a1);
        push(x0b,-h/2,z0b,nx0,0,nz0); push(x1b,-h/2,z1b,nx1,0,nz1); push(x0t,h/2,z0t,nx0,0,nz0);
        push(x1b,-h/2,z1b,nx1,0,nz1); push(x1t,h/2,z1t,nx1,0,nz1); push(x0t,h/2,z0t,nx0,0,nz0);
    }
    // caps
    for(int i=0;i<sec;i++){
        float a0=2*PI*i/sec, a1=2*PI*(i+1)/sec;
        // top cap
        push(0,h/2,0, 0,1,0);
        push(rTop*cosf(a0),h/2,rTop*sinf(a0), 0,1,0);
        push(rTop*cosf(a1),h/2,rTop*sinf(a1), 0,1,0);
        // bottom cap
        push(0,-h/2,0, 0,-1,0);
        push(rBot*cosf(a1),-h/2,rBot*sinf(a1), 0,-1,0);
        push(rBot*cosf(a0),-h/2,rBot*sinf(a0), 0,-1,0);
    }
    return uploadMesh(v);
}

// ---- Sphere ----
static Mesh makeSphere(float r=0.5f, int stacks=10, int slices=10) {
    std::vector<float> v;
    const float PI = 3.14159265f;
    for(int i=0;i<stacks;i++){
        float phi0=PI*i/stacks-PI/2, phi1=PI*(i+1)/stacks-PI/2;
        for(int j=0;j<slices;j++){
            float th0=2*PI*j/slices, th1=2*PI*(j+1)/slices;
            auto pt=[&](float ph, float th){
                float x=r*cosf(ph)*cosf(th), y=r*sinf(ph), z=r*cosf(ph)*sinf(th);
                v.insert(v.end(),{x,y,z, x/r,y/r,z/r});
            };
            pt(phi0,th0); pt(phi0,th1); pt(phi1,th0);
            pt(phi1,th0); pt(phi0,th1); pt(phi1,th1);
        }
    }
    return uploadMesh(v);
}

// ---- Cone ----
static Mesh makeCone(float r=0.5f, float h=1.0f, int sec=12) {
    return makeCylinder(0.0f, r, h, sec);
}

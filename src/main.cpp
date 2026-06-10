#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// stb_image untuk load texture
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

#include "Shader.h"
#include "Mesh.h"

const int SCR_W = 1280;
const int SCR_H = 720;
const int GRID  = 60;

// ── Input ─────────────────────────────────────────────────────
bool kW=false, kA=false, kS=false, kD=false;

void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}
void key_callback(GLFWwindow* win, int key, int, int action, int) {
    bool p=(action==GLFW_PRESS), r=(action==GLFW_RELEASE);
    if(key==GLFW_KEY_W||key==GLFW_KEY_UP)    {if(p)kW=true;if(r)kW=false;}
    if(key==GLFW_KEY_S||key==GLFW_KEY_DOWN)  {if(p)kS=true;if(r)kS=false;}
    if(key==GLFW_KEY_A||key==GLFW_KEY_LEFT)  {if(p)kA=true;if(r)kA=false;}
    if(key==GLFW_KEY_D||key==GLFW_KEY_RIGHT) {if(p)kD=true;if(r)kD=false;}
    if(key==GLFW_KEY_ESCAPE&&p) glfwSetWindowShouldClose(win,true);
}

// ── Texture loader ────────────────────────────────────────────
unsigned int loadTexture(const char* path) {
    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // Wrapping: repeat texture tile
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Filtering: linear for smooth look
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load image
    int w, h, nChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &nChannels, 0);
    if (data) {
        GLenum fmt = (nChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Texture loaded: " << path << " (" << w << "x" << h << ")\n";
    } else {
        std::cerr << "Failed to load texture: " << path << "\n";
    }
    stbi_image_free(data);
    return texID;
}

// ── Upload mesh dengan UV coords (pos + normal + uv) ──────────
// Format: x,y,z, nx,ny,nz, u,v  (8 floats per vertex)
struct MeshUV {
    unsigned int VAO, VBO;
    int vertCount;
    void draw() const {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertCount);
        glBindVertexArray(0);
    }
};

MeshUV uploadMeshUV(const std::vector<float>& data) {
    MeshUV m;
    m.vertCount = (int)data.size() / 8;
    glGenVertexArrays(1, &m.VAO);
    glGenBuffers(1, &m.VBO);
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(float), data.data(), GL_STATIC_DRAW);
    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return m;
}

int main() {
    // ── GLFW ──────────────────────────────────────────────────
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* win = glfwCreateWindow(SCR_W, SCR_H, "BrushBound", nullptr, nullptr);
    if(!win){ std::cerr<<"Failed to create window\n"; return -1; }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetKeyCallback(win, key_callback);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cerr<<"Failed to init GLAD\n"; return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glViewport(0, 0, SCR_W, SCR_H);

    // ── Shader ────────────────────────────────────────────────
    Shader shader("shaders/basic.vert", "shaders/basic.frag");

    // ── Load textures ─────────────────────────────────────────
    unsigned int texDirt = loadTexture("textures/dirt.jpg");
    unsigned int texWall = loadTexture("textures/wall.png");

    // ── Floor mesh (dengan UV) ────────────────────────────────
    // UV 0..1 satu kali = gambar penuh, tidak tiled
    float gs = (float)GRID;
    std::vector<float> floorV = {
        // x        y   z        nx ny nz   u   v
        -0.5f,    0, -0.5f,    0, 1, 0,   0,  0,
         gs-0.5f, 0, -0.5f,    0, 1, 0,   1,  0,
         gs-0.5f, 0,  gs-0.5f, 0, 1, 0,   1,  1,
        -0.5f,    0, -0.5f,    0, 1, 0,   0,  0,
         gs-0.5f, 0,  gs-0.5f, 0, 1, 0,   1,  1,
        -0.5f,    0,  gs-0.5f, 0, 1, 0,   0,  1,
    };
    MeshUV mFloor = uploadMeshUV(floorV);

    // ── Wall mesh (dengan UV) ─────────────────────────────────
    std::vector<float> wallV;
    float wh = 4.0f;   // tinggi tembok
    float wt = 0.8f;   // tebal tembok

    auto addWallUV = [&](
        float x0, float z0, float x1, float z1,
        float nx, float nz,
        float uLen  // panjang UV horizontal
    ){
        // Inner face (menghadap arena)
        wallV.insert(wallV.end(), {
            x0, 0,  z0,  nx,0,nz,  0,    0,
            x1, 0,  z1,  nx,0,nz,  uLen, 0,
            x1, wh, z1,  nx,0,nz,  uLen, 1,
            x0, 0,  z0,  nx,0,nz,  0,    0,
            x1, wh, z1,  nx,0,nz,  uLen, 1,
            x0, wh, z0,  nx,0,nz,  0,    1,
        });
        // Top face
        wallV.insert(wallV.end(), {
            x0, wh, z0,  0,1,0,  0,    0,
            x1, wh, z1,  0,1,0,  uLen, 0,
            x1, wh, z1,  0,1,0,  uLen, 1,
            x0, wh, z0,  0,1,0,  0,    0,
            x1, wh, z1,  0,1,0,  uLen, 1,
            x0, wh, z0,  0,1,0,  0,    1,
        });
    };

    // UV 1.0 = satu gambar utuh per sisi tembok
    // Selatan (z = -0.5, normal +Z)
    addWallUV(-0.5f, -0.5f,    gs-0.5f, -0.5f,    0, 1, 1.0f);
    // Utara (z = gs-0.5, normal -Z)
    addWallUV(-0.5f, gs-0.5f,  gs-0.5f,  gs-0.5f, 0,-1, 1.0f);
    // Barat (x = -0.5, normal +X)
    addWallUV(-0.5f, -0.5f,   -0.5f,    gs-0.5f,  1, 0, 1.0f);
    // Timur (x = gs-0.5, normal -X)
    addWallUV(gs-0.5f,-0.5f,  gs-0.5f,  gs-0.5f, -1, 0, 1.0f);

    MeshUV mWall = uploadMeshUV(wallV);

    // ── Meshes untuk wizard ───────────────────────────────────
    Mesh mSphere   = makeSphere(0.5f,12,12);
    Mesh mCylinder = makeCylinder(0.5f,0.5f,1.0f,14);
    Mesh mCone     = makeCone(0.5f,1.0f,14);

    // ── Player state ──────────────────────────────────────────
    glm::vec3 playerPos((float)GRID/2, 0.0f, (float)GRID/2);
    float     playerAngle = 0.0f;
    float     playerSpeed = 14.0f;

    // Camera
    glm::vec3 camPos(playerPos.x, 5.0f, playerPos.z - 8.0f);
    glm::vec3 camTarget = playerPos;

    float lastTime = (float)glfwGetTime();

    std::cout << "BrushBound — WASD/Arrows: gerak, ESC: keluar\n";

    while(!glfwWindowShouldClose(win)) {
        float now = (float)glfwGetTime();
        float dt  = std::min(now - lastTime, 0.05f);
        lastTime  = now;

        glfwPollEvents();

        // ── Movement ──────────────────────────────────────────
        glm::vec3 moveDir(0.0f);
        if(kW) moveDir += glm::vec3( sinf(playerAngle), 0,  cosf(playerAngle));
        if(kS) moveDir += glm::vec3(-sinf(playerAngle), 0, -cosf(playerAngle));
        if(kA) playerAngle += 2.5f * dt;
        if(kD) playerAngle -= 2.5f * dt;

        if(glm::length(moveDir) > 0.001f)
            moveDir = glm::normalize(moveDir);

        glm::vec3 newPos = playerPos + moveDir * playerSpeed * dt;
        float margin = 1.0f;
        newPos.x = glm::clamp(newPos.x, margin, (float)GRID-1.0f-margin);
        newPos.z = glm::clamp(newPos.z, margin, (float)GRID-1.0f-margin);
        playerPos = newPos;

        // ── Camera ────────────────────────────────────────────
        float camDist=8.0f, camHeight=5.0f;
        glm::vec3 targetCamPos(
            playerPos.x - sinf(playerAngle)*camDist,
            playerPos.y + camHeight,
            playerPos.z - cosf(playerAngle)*camDist
        );
        glm::vec3 targetLook(playerPos.x, playerPos.y+0.5f, playerPos.z);
        float lf = 1.0f - expf(-8.0f * dt);
        camPos    = glm::mix(camPos,    targetCamPos, lf);
        camTarget = glm::mix(camTarget, targetLook,   lf);

        // ── Render ────────────────────────────────────────────
        glClearColor(0.4f, 0.65f, 0.9f, 1.0f); // langit biru
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(glm::radians(60.0f),
            (float)SCR_W/SCR_H, 0.1f, 300.0f);
        glm::mat4 view = glm::lookAt(camPos, camTarget, glm::vec3(0,1,0));

        shader.use();
        shader.setMat4("projection", proj);
        shader.setMat4("view", view);
        shader.setVec3("viewPos", camPos);
        shader.setVec3("lightPos", glm::vec3((float)GRID/2, 30.0f, (float)GRID/2));
        shader.setVec3("lightColor", glm::vec3(1.0f, 0.95f, 0.85f)); // cahaya hangat
        shader.setFloat("alpha", 1.0f);

        // ── Draw floor (texture dirt) ─────────────────────────
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texDirt);
        shader.use();
        glUniform1i(glGetUniformLocation(shader.ID, "texSampler"), 0);
        glUniform1i(glGetUniformLocation(shader.ID, "useTexture"), 1);
        {
            glm::mat4 m = glm::mat4(1.0f);
            shader.setMat4("model", m);
            shader.setFloat("emissive", 0.0f);
            mFloor.draw();
        }

        // ── Draw wall (texture wall.jpg) ──────────────────────
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texWall);
        glUniform1i(glGetUniformLocation(shader.ID, "texSampler"), 0);
        glUniform1i(glGetUniformLocation(shader.ID, "useTexture"), 1);
        {
            glm::mat4 m = glm::mat4(1.0f);
            shader.setMat4("model", m);
            shader.setFloat("emissive", 0.0f);
            mWall.draw();
        }

        // ── Draw wizard ───────────────────────────────────────
        glUniform1i(glGetUniformLocation(shader.ID, "useTexture"), 0);

        float bob  = sinf(now * 4.0f) * 0.05f;
        glm::vec3 base = playerPos + glm::vec3(0, 0.5f+bob, 0);
        glm::mat4 rotY = glm::rotate(glm::mat4(1), playerAngle, glm::vec3(0,1,0));

        // Kepala
        {
            glm::mat4 m = glm::translate(glm::mat4(1), base+glm::vec3(0,0.55f,0));
            m = glm::scale(m, glm::vec3(0.28f));
            shader.setMat4("model", m);
            shader.setVec3("objectColor", glm::vec3(0.95f,0.85f,0.75f));
            shader.setFloat("emissive", 0.0f);
            mSphere.draw();
        }
        // Topi
        {
            glm::mat4 m = glm::translate(glm::mat4(1), base+glm::vec3(0,0.85f,0));
            m = glm::scale(m, glm::vec3(0.22f,0.45f,0.22f));
            shader.setMat4("model", m);
            shader.setVec3("objectColor", glm::vec3(0.2f,0.0f,0.8f));
            shader.setFloat("emissive", 0.3f);
            mCone.draw();
        }
        // Jubah
        {
            glm::mat4 m = glm::translate(glm::mat4(1), base+glm::vec3(0,0.1f,0));
            m = m * rotY;
            m = glm::scale(m, glm::vec3(0.38f,0.55f,0.38f));
            shader.setMat4("model", m);
            shader.setVec3("objectColor", glm::vec3(0.15f,0.0f,0.55f));
            shader.setFloat("emissive", 0.1f);
            mCylinder.draw();
        }
        // Gagang sapu
        {
            glm::mat4 m = glm::translate(glm::mat4(1), base+glm::vec3(0,-0.10f,0));
            m = m * rotY;
            m = glm::rotate(m, glm::radians(90.0f), glm::vec3(1,0,0));
            m = glm::scale(m, glm::vec3(0.06f,1.1f,0.06f));
            shader.setMat4("model", m);
            shader.setVec3("objectColor", glm::vec3(0.6f,0.4f,0.1f));
            shader.setFloat("emissive", 0.0f);
            mCylinder.draw();
        }
        // Bulu sapu
        {
            glm::mat4 m = glm::translate(glm::mat4(1), base+glm::vec3(0,-0.10f,0));
            m = m * rotY;
            m = glm::translate(m, glm::vec3(0,0,-0.58f));
            m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1,0,0));
            m = glm::scale(m, glm::vec3(0.20f,0.30f,0.20f));
            shader.setMat4("model", m);
            shader.setVec3("objectColor", glm::vec3(0.5f,0.35f,0.08f));
            shader.setFloat("emissive", 0.0f);
            mCone.draw();
        }
        // Shadow bawah wizard
        {
            glm::mat4 m = glm::translate(glm::mat4(1),
                glm::vec3(playerPos.x, 0.02f, playerPos.z));
            m = glm::scale(m, glm::vec3(0.5f,0.01f,0.5f));
            shader.setMat4("model", m);
            shader.setVec3("objectColor", glm::vec3(0,0,0));
            shader.setFloat("emissive", 0.0f);
            mCylinder.draw();
        }

        glfwSwapBuffers(win);
    }

    glfwTerminate();
    return 0;
}
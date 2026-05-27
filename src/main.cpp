#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

#include "Shader.h"
#include "Mesh.h"

const int SCR_W = 1280;
const int SCR_H = 720;
const int GRID  = 60;

// ── Input state ───────────────────────────────────────────────
bool kW=false, kA=false, kS=false, kD=false;

void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

void key_callback(GLFWwindow* win, int key, int, int action, int) {
    bool p = (action == GLFW_PRESS), r = (action == GLFW_RELEASE);
    if (key==GLFW_KEY_W||key==GLFW_KEY_UP)    { if(p) kW=true; if(r) kW=false; }
    if (key==GLFW_KEY_S||key==GLFW_KEY_DOWN)  { if(p) kS=true; if(r) kS=false; }
    if (key==GLFW_KEY_A||key==GLFW_KEY_LEFT)  { if(p) kA=true; if(r) kA=false; }
    if (key==GLFW_KEY_D||key==GLFW_KEY_RIGHT) { if(p) kD=true; if(r) kD=false; }
    if (key==GLFW_KEY_ESCAPE && p) glfwSetWindowShouldClose(win, true);
}

int main() {
    // ── Init GLFW ─────────────────────────────────────────────
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* win = glfwCreateWindow(SCR_W, SCR_H, "BrushBound", nullptr, nullptr);
    if (!win) { std::cerr << "Failed to create window\n"; return -1; }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetKeyCallback(win, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n"; return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glViewport(0, 0, SCR_W, SCR_H);

    // ── Shader ────────────────────────────────────────────────
    Shader shader("shaders/basic.vert", "shaders/basic.frag");

    // ── Arena meshes ──────────────────────────────────────────
    // Floor
    std::vector<float> floorV = {
        -0.5f,0,-0.5f, 0,1,0,
        (float)GRID-0.5f,0,-0.5f, 0,1,0,
        (float)GRID-0.5f,0,(float)GRID-0.5f, 0,1,0,
        -0.5f,0,-0.5f, 0,1,0,
        (float)GRID-0.5f,0,(float)GRID-0.5f, 0,1,0,
        -0.5f,0,(float)GRID-0.5f, 0,1,0,
    };
    Mesh mFloor = uploadMesh(floorV);

    // Grid lines
    std::vector<float> gridV;
    float gy=0.01f, thick=0.06f;
    for(int i=0;i<=GRID;i++){
        float p=(float)i-0.5f, lo=-0.5f, hi=(float)GRID-0.5f;
        gridV.insert(gridV.end(),{
            p-thick,gy,lo, 0,1,0,  p+thick,gy,lo, 0,1,0,  p+thick,gy,hi, 0,1,0,
            p-thick,gy,lo, 0,1,0,  p+thick,gy,hi, 0,1,0,  p-thick,gy,hi, 0,1,0,
            lo,gy,p-thick, 0,1,0,  hi,gy,p-thick, 0,1,0,  hi,gy,p+thick, 0,1,0,
            lo,gy,p-thick, 0,1,0,  hi,gy,p+thick, 0,1,0,  lo,gy,p+thick, 0,1,0,
        });
    }
    Mesh mGrid = uploadMesh(gridV);

    // Border walls
    std::vector<float> wallV;
    float wh=1.2f, gs=(float)GRID, wt=0.15f;
    auto addWall=[&](float x0,float z0,float x1,float z1){
        // top face
        wallV.insert(wallV.end(),{
            x0,wh,z0, 0,1,0,  x1,wh,z0, 0,1,0,  x1,wh,z1, 0,1,0,
            x0,wh,z0, 0,1,0,  x1,wh,z1, 0,1,0,  x0,wh,z1, 0,1,0,
        });
        // inner face (facing arena)
        float nx=0,nz=0;
        if(z0==z1){ nz=(z0<0)?1:-1; } else { nx=(x0<0)?1:-1; }
        wallV.insert(wallV.end(),{
            x0,0,z0, nx,0,nz,  x1,0,z0, nx,0,nz,  x1,wh,z0, nx,0,nz,
            x0,0,z0, nx,0,nz,  x1,wh,z0, nx,0,nz,  x0,wh,z0, nx,0,nz,
            x0,0,z1, nx,0,nz,  x1,0,z1, nx,0,nz,  x1,wh,z1, nx,0,nz,
            x0,0,z1, nx,0,nz,  x1,wh,z1, nx,0,nz,  x0,wh,z1, nx,0,nz,
        });
    };
    addWall(-0.5f,-0.5f,     gs-0.5f,-0.5f+wt);
    addWall(-0.5f,gs-0.5f-wt,gs-0.5f,gs-0.5f);
    addWall(-0.5f,-0.5f,     -0.5f+wt,gs-0.5f);
    addWall(gs-0.5f-wt,-0.5f,gs-0.5f,gs-0.5f);
    Mesh mWall = uploadMesh(wallV);

    // Meshes for player character
    Mesh mSphere   = makeSphere(0.5f,12,12);
    Mesh mCylinder = makeCylinder(0.5f,0.5f,1.0f,14);
    Mesh mCone     = makeCone(0.5f,1.0f,14);
    Mesh mCube     = makeCube();

    // ── Player state ──────────────────────────────────────────
    // Smooth world position (free movement, not grid-locked yet)
    glm::vec3 playerPos((float)GRID/2, 0.0f, (float)GRID/2);
    float     playerAngle = 0.0f;   // facing direction (radians, Y axis)
    float     playerSpeed = 14.0f;   // units per second

    // Camera smoothing
    glm::vec3 camPos(playerPos.x, 5.0f, playerPos.z - 8.0f);
    glm::vec3 camTarget = playerPos;

    float lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(win)) {
        float now = (float)glfwGetTime();
        float dt  = std::min(now - lastTime, 0.05f);
        lastTime  = now;

        glfwPollEvents();

        // ── Player movement ───────────────────────────────────
        glm::vec3 moveDir(0.0f);

        if (kW) moveDir += glm::vec3( sinf(playerAngle), 0,  cosf(playerAngle));
        if (kS) moveDir += glm::vec3(-sinf(playerAngle), 0, -cosf(playerAngle));
        if (kA) playerAngle += 2.5f * dt;   // turn left
        if (kD) playerAngle -= 2.5f * dt;   // turn right

        if (glm::length(moveDir) > 0.001f)
            moveDir = glm::normalize(moveDir);

        glm::vec3 newPos = playerPos + moveDir * playerSpeed * dt;

        // Clamp inside arena bounds
        float margin = 0.6f;
        newPos.x = glm::clamp(newPos.x, margin, (float)GRID - 1.0f - margin);
        newPos.z = glm::clamp(newPos.z, margin, (float)GRID - 1.0f - margin);
        playerPos = newPos;

        // ── Camera: third-person behind player ────────────────
        float camDist   = 8.0f;
        float camHeight = 5.0f;

        // Target position: behind player based on facing angle
        glm::vec3 targetCamPos(
            playerPos.x - sinf(playerAngle) * camDist,
            playerPos.y + camHeight,
            playerPos.z - cosf(playerAngle) * camDist
        );
        glm::vec3 targetLook(
            playerPos.x,
            playerPos.y + 0.5f,
            playerPos.z
        );

        // Smooth lerp camera
        float lerpSpeed = 8.0f;
        float lf = 1.0f - expf(-lerpSpeed * dt);
        camPos    = glm::mix(camPos,    targetCamPos, lf);
        camTarget = glm::mix(camTarget, targetLook,   lf);

        // ── Render ────────────────────────────────────────────
        glClearColor(0.05f, 0.02f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(glm::radians(60.0f),
            (float)SCR_W/SCR_H, 0.1f, 200.0f);
        glm::mat4 view = glm::lookAt(camPos, camTarget, glm::vec3(0,1,0));

        shader.use();
        shader.setMat4("projection", proj);
        shader.setMat4("view", view);
        shader.setVec3("viewPos", camPos);
        shader.setVec3("lightPos", glm::vec3(playerPos.x, 15.0f, playerPos.z));
        shader.setVec3("lightColor", glm::vec3(1,1,1));
        shader.setFloat("alpha", 1.0f);

        // Floor
        {
            shader.setMat4("model", glm::mat4(1));
            shader.setVec3("objectColor", glm::vec3(0.10f,0.05f,0.20f));
            shader.setFloat("emissive", 0.0f);
            mFloor.draw();
        }
        // Grid
        {
            shader.setMat4("model", glm::mat4(1));
            shader.setVec3("objectColor", glm::vec3(0.25f,0.10f,0.45f));
            shader.setFloat("emissive", 0.15f);
            mGrid.draw();
        }
        // Walls
        {
            shader.setMat4("model", glm::mat4(1));
            shader.setVec3("objectColor", glm::vec3(0.3f,0.0f,0.8f));
            shader.setFloat("emissive", 0.7f);
            mWall.draw();
        }

        // ── Player character (wizard) ─────────────────────────
        float bob = sinf(now * 4.0f) * 0.05f;
        glm::vec3 base = playerPos + glm::vec3(0, 0.5f + bob, 0);
        glm::mat4 rotY = glm::rotate(glm::mat4(1), playerAngle, glm::vec3(0,1,0));

        // Head
        {
            glm::mat4 m = glm::translate(glm::mat4(1), base + glm::vec3(0,0.55f,0));
            m = glm::scale(m, glm::vec3(0.28f));
            shader.setMat4("model", m);
            shader.setVec3("objectColor", glm::vec3(0.95f,0.85f,0.75f));
            shader.setFloat("emissive", 0.0f);
            mSphere.draw();
        }
        // Hat
        {
            glm::mat4 m = glm::translate(glm::mat4(1), base + glm::vec3(0,0.85f,0));
            m = glm::scale(m, glm::vec3(0.22f,0.45f,0.22f));
            shader.setMat4("model", m);
            shader.setVec3("objectColor", glm::vec3(0.2f,0.0f,0.8f));
            shader.setFloat("emissive", 0.4f);
            mCone.draw();
        }
        // Robe
        {
            glm::mat4 m = glm::translate(glm::mat4(1), base + glm::vec3(0,0.1f,0));
            m = m * rotY;
            m = glm::scale(m, glm::vec3(0.38f,0.55f,0.38f));
            shader.setMat4("model", m);
            shader.setVec3("objectColor", glm::vec3(0.15f,0.0f,0.55f));
            shader.setFloat("emissive", 0.1f);
            mCylinder.draw();
        }
        // Broomstick handle -- lies flat along Z (forward direction)
        {
            glm::mat4 m = glm::translate(glm::mat4(1), base + glm::vec3(0,-0.10f,0));
            m = m * rotY;
            // rotate cylinder (default Y-up) to lie along Z axis (forward)
            m = glm::rotate(m, glm::radians(90.0f), glm::vec3(1,0,0));
            m = glm::scale(m, glm::vec3(0.06f, 1.1f, 0.06f));
            shader.setMat4("model", m);
            shader.setVec3("objectColor", glm::vec3(0.6f,0.4f,0.1f));
            shader.setFloat("emissive", 0.0f);
            mCylinder.draw();
        }
        // Broom bristles -- at FRONT end of stick (-Z = in front of player)
        {
            glm::mat4 m = glm::translate(glm::mat4(1), base + glm::vec3(0,-0.10f,0));
            m = m * rotY;
            // bristles at front end of broom (-Z direction = forward)
            m = glm::translate(m, glm::vec3(0, 0, -0.58f));
            m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1,0,0));
            m = glm::scale(m, glm::vec3(0.20f, 0.30f, 0.20f));
            shader.setMat4("model", m);
            shader.setVec3("objectColor", glm::vec3(0.5f,0.35f,0.08f));
            shader.setFloat("emissive", 0.0f);
            mCone.draw();
        }

        // ── Shadow (flat circle under wizard) ─────────────────
        {
            glm::mat4 m = glm::translate(glm::mat4(1),
                glm::vec3(playerPos.x, 0.02f, playerPos.z));
            m = glm::scale(m, glm::vec3(0.5f, 0.01f, 0.5f));
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
// ── main.cpp ─────────────────────────────────────────────────────
// Hanya berisi: inisialisasi GLFW/GL, setup mesh & texture,
// game loop, dan render. Semua logika game ada di Game.h.
// ─────────────────────────────────────────────────────────────────
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include "Mesh.h"
#include "Shader.h"
#include "UI.h"
#include "Game.h"

// ── Konstanta layar ───────────────────────────────────────────────
static constexpr int SCR_W = 1280;
static constexpr int SCR_H = 720;

// ── MeshUV (pos + normal + uv, 8 float per vertex) ───────────────
struct MeshUV {
    unsigned int VAO = 0, VBO = 0;
    int vertCount = 0;
    void draw() const {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertCount);
        glBindVertexArray(0);
    }
};

static MeshUV uploadMeshUV(const std::vector<float>& data) {
    MeshUV m;
    m.vertCount = (int)data.size() / 8;
    glGenVertexArrays(1, &m.VAO);
    glGenBuffers(1, &m.VBO);
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return m;
}

// ── Texture loader ────────────────────────────────────────────────
static unsigned int loadTexture(const char* path) {
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int w, h, ch;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &ch, 0);
    if (data) {
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Texture: " << path << " (" << w << "x" << h << ")\n";
    } else {
        std::cerr << "Gagal load texture: " << path << "\n";
    }
    stbi_image_free(data);
    return id;
}

// ── Input globals ─────────────────────────────────────────────────
static bool   kA = false, kD = false;
static bool   mouseClicked = false;
static double mouseX = 0.0, mouseY = 0.0;

static void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

static void key_callback(GLFWwindow* win, int key, int, int action, int) {
    bool p = (action == GLFW_PRESS), r = (action == GLFW_RELEASE);
    if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT)  { if (p) kA = true; if (r) kA = false; }
    if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) { if (p) kD = true; if (r) kD = false; }
    if (key == GLFW_KEY_ESCAPE && p) glfwSetWindowShouldClose(win, true);
}

static void mouse_button_callback(GLFWwindow* win, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        mouseClicked = true;
        glfwGetCursorPos(win, &mouseX, &mouseY);
    }
}

// ── Build mesh arena ──────────────────────────────────────────────
static MeshUV buildFloor(float gs) {
    std::vector<float> v = {
        -0.5f,  0, -0.5f,  0,1,0, 0,0,
         gs-.5f,0, -0.5f,  0,1,0, 1,0,
         gs-.5f,0,  gs-.5f,0,1,0, 1,1,
        -0.5f,  0, -0.5f,  0,1,0, 0,0,
         gs-.5f,0,  gs-.5f,0,1,0, 1,1,
        -0.5f,  0,  gs-.5f,0,1,0, 0,1,
    };
    return uploadMeshUV(v);
}

static MeshUV buildWalls(float gs) {
    // Wall height & tile size sama seperti Beta (tiling proporsional)
    const float wh       = 4.0f;
    const float wallTile = 4.0f;
    std::vector<float> v;

    auto addSide = [&](float x0, float z0, float x1, float z1, float nx, float nz) {
        float len  = sqrtf((x1-x0)*(x1-x0)+(z1-z0)*(z1-z0));
        float uLen = len / wallTile;
        // Inner face
        v.insert(v.end(), {
            x0,  0,  z0, nx,0,nz,    0, 0,
            x1,  0,  z1, nx,0,nz, uLen, 0,
            x1, wh,  z1, nx,0,nz, uLen, 1,
            x0,  0,  z0, nx,0,nz,    0, 0,
            x1, wh,  z1, nx,0,nz, uLen, 1,
            x0, wh,  z0, nx,0,nz,    0, 1,
        });
        // Top cap
        v.insert(v.end(), {
            x0, wh,  z0, 0,1,0,    0, 0,
            x1, wh,  z1, 0,1,0, uLen, 0,
            x1, wh,  z1, 0,1,0, uLen, 1,
            x0, wh,  z0, 0,1,0,    0, 0,
            x1, wh,  z1, 0,1,0, uLen, 1,
            x0, wh,  z0, 0,1,0,    0, 1,
        });
    };

    addSide(-0.5f,   -0.5f,   gs-0.5f, -0.5f,    0,  1); // Selatan
    addSide(-0.5f,   gs-0.5f, gs-0.5f,  gs-0.5f, 0, -1); // Utara
    addSide(-0.5f,   -0.5f,  -0.5f,    gs-0.5f,  1,  0); // Barat
    addSide(gs-0.5f, -0.5f,  gs-0.5f,  gs-0.5f, -1,  0); // Timur

    return uploadMeshUV(v);
}

// ── Render satu wizard + trailnya ────────────────────────────────
static void renderWizard(const Wizard& w, Shader& shader, Mesh& mCyl) {
    glUniform1i(glGetUniformLocation(shader.ID, "useTexture"), 0);
    // Trail pakai alpha fade — butuh blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    w.trail.draw(shader, mCyl);
    glDisable(GL_BLEND);
    w.draw(shader);
}

// ── Helper: gambar tombol dengan teks ────────────────────────────
static void drawButton(UIQuad& quad, Shader& sh, const glm::mat4& proj,
                       float x, float y, float w, float h,
                       glm::vec3 col, const std::string& label, bool hover)
{
    // Shadow tipis
    quad.draw(sh, proj, x+4, y-4, w, h, glm::vec3(0,0,0), 0.4f);
    // Badan tombol
    quad.draw(sh, proj, x, y, w, h, col, hover ? 1.0f : 0.88f);
    // Border atas (highlight)
    quad.draw(sh, proj, x, y+h-3, w, 3, glm::vec3(1,1,1), 0.25f);

    // Teks di tengah
    const float cH = 18.0f, cW = cH * 0.55f, cGap = 4.0f;
    float tw = textWidth(label, cW, cGap);
    float tx = x + w/2.0f - tw/2.0f;
    float ty = y + h/2.0f + cH/2.0f;
    // Shadow teks
    drawText(quad, sh, proj, label, tx+1, ty-1, cW, cH, cGap, glm::vec3(0,0,0));
    drawText(quad, sh, proj, label, tx,   ty,   cW, cH, cGap, glm::vec3(1,1,1));
}

// ── Helper: gambar panel dengan border ───────────────────────────
static void drawPanel(UIQuad& quad, Shader& sh, const glm::mat4& proj,
                      float x, float y, float w, float h,
                      glm::vec3 bgCol, float bgAlpha,
                      glm::vec3 borderCol = glm::vec3(1,1,1), float borderAlpha = 0.15f)
{
    // Border
    const float b = 2.0f;
    quad.draw(sh, proj, x-b, y-b, w+b*2, h+b*2, borderCol, borderAlpha);
    // Background
    quad.draw(sh, proj, x, y, w, h, bgCol, bgAlpha);
}

// ── Render UI overlay ─────────────────────────────────────────────
static void renderUI(const Game& game, UIQuad& quad, Shader& uiShader,
                     const glm::mat4& proj,
                     float btnX, float btnY, float btnW, float btnH,
                     bool hover)
{
    const float CX = SCR_W / 2.0f;
    const float CY = SCR_H / 2.0f;

    // ── HUD skor & level — tampil saat PLAYING ────────────────────
    if (game.state == GameState::PLAYING) {
        const float margin = 20.0f;
        const float dH = 32.0f, dW = dH * 0.55f, dGap = 7.0f;
        const float panelH = dH + 22.0f, panelW = 200.0f;

        // Panel skor (kanan atas)
        drawPanel(quad, uiShader, proj,
            SCR_W - margin - panelW, SCR_H - margin - panelH,
            panelW, panelH, glm::vec3(0,0,0), 0.45f,
            glm::vec3(1.0f, 0.95f, 0.2f), 0.4f);
        drawNumber(quad, uiShader, proj, game.score,
            SCR_W - margin - 14.0f, SCR_H - margin,
            dW, dH, dGap, glm::vec3(1.0f, 0.95f, 0.2f));

        // Label "SCORE" kecil di atas angka
        const float lH = 11.0f, lW = lH * 0.55f, lGap = 3.0f;
        float lw = textWidth("SCORE", lW, lGap);
        drawText(quad, uiShader, proj, "SCORE",
            SCR_W - margin - panelW/2.0f - lw/2.0f,
            SCR_H - margin - 2.0f,
            lW, lH, lGap, glm::vec3(1.0f, 0.95f, 0.2f));

        // Panel level (kiri atas)
        std::string lvlStr = "LEVEL  " + std::to_string(game.level);
        const float lcH = 20.0f, lcW = lcH * 0.55f, lcGap = 5.0f;
        float lvlPanelW = textWidth(lvlStr, lcW, lcGap) + 32.0f;
        float lvlPanelH = panelH;
        float lvlPanelX = margin;
        float lvlPanelY = SCR_H - margin - lvlPanelH;
        drawPanel(quad, uiShader, proj,
            lvlPanelX, lvlPanelY, lvlPanelW, lvlPanelH,
            glm::vec3(0,0,0), 0.45f, glm::vec3(0.2f, 0.85f, 0.95f), 0.4f);
        drawText(quad, uiShader, proj, lvlStr,
            lvlPanelX + 16.0f,
            lvlPanelY + lvlPanelH/2.0f + lcH/2.0f,
            lcW, lcH, lcGap, glm::vec3(0.2f, 0.85f, 0.95f));
    }

    switch (game.state) {

    // ── MENU ──────────────────────────────────────────────────────
    case GameState::MENU: {
        // Overlay gelap
        quad.draw(uiShader, proj, 0, 0, SCR_W, SCR_H, glm::vec3(0, 0, 0.06f), 0.62f);

        // Garis dekoratif atas & bawah
        quad.draw(uiShader, proj, 0, SCR_H - 4, SCR_W, 4, glm::vec3(0.2f,0.0f,0.8f), 0.9f);
        quad.draw(uiShader, proj, 0, 0, SCR_W, 4, glm::vec3(0.2f,0.0f,0.8f), 0.9f);

        // Judul: "BRUSH" baris atas
        {
            const float tH = 72.0f, tW = tH * 0.55f, tGap = 10.0f;
            float tw = textWidth("BRUSH", tW, tGap);
            // Shadow
            drawText(quad, uiShader, proj, "BRUSH",
                CX - tw/2.0f + 3, CY + 90.0f - 3, tW, tH, tGap,
                glm::vec3(0.1f, 0.0f, 0.4f));
            drawText(quad, uiShader, proj, "BRUSH",
                CX - tw/2.0f, CY + 90.0f, tW, tH, tGap,
                glm::vec3(0.55f, 0.2f, 1.0f));
        }
        // Judul: "BOUND" baris bawah, warna berbeda
        {
            const float tH = 72.0f, tW = tH * 0.55f, tGap = 10.0f;
            float tw = textWidth("BOUND", tW, tGap);
            drawText(quad, uiShader, proj, "BOUND",
                CX - tw/2.0f + 3, CY + 10.0f - 3, tW, tH, tGap,
                glm::vec3(0.0f, 0.05f, 0.25f));
            drawText(quad, uiShader, proj, "BOUND",
                CX - tw/2.0f, CY + 10.0f, tW, tH, tGap,
                glm::vec3(0.2f, 0.75f, 1.0f));
        }

        // Garis pembatas di bawah judul
        quad.draw(uiShader, proj, CX - 120, CY - 10, 240, 2,
            glm::vec3(0.4f, 0.2f, 0.8f), 0.5f);

        // Subteks kecil
        {
            const float sH = 13.0f, sW = sH * 0.55f, sGap = 3.5f;
            std::string sub = "USE  A  -  D  TO  STEER";
            float sw = textWidth(sub, sW, sGap);
            drawText(quad, uiShader, proj, sub,
                CX - sw/2.0f, CY - 28.0f, sW, sH, sGap,
                glm::vec3(0.5f, 0.5f, 0.7f));
        }

        // Tombol START GAME
        glm::vec3 btnCol = hover
            ? glm::vec3(0.35f, 0.85f, 0.45f)
            : glm::vec3(0.15f, 0.65f, 0.28f);
        drawButton(quad, uiShader, proj,
            btnX, btnY - 30.0f, btnW, btnH,
            btnCol, "START  GAME", hover);
        break;
    }

    // ── COUNTDOWN ─────────────────────────────────────────────────
    case GameState::COUNTDOWN: {
        quad.draw(uiShader, proj, 0, 0, SCR_W, SCR_H, glm::vec3(0,0,0.05f), 0.38f);

        float t = game.countdown;
        std::string label; glm::vec3 col;
        if      (t < 1.0f) { label = "3"; col = glm::vec3(1.0f, 0.3f, 0.3f); }
        else if (t < 2.0f) { label = "2"; col = glm::vec3(1.0f, 0.7f, 0.2f); }
        else if (t < 3.0f) { label = "1"; col = glm::vec3(1.0f, 0.95f, 0.2f); }
        else               { label = "GO"; col = glm::vec3(0.3f, 0.95f, 0.4f); }

        float tInSeg = fmodf(t, 1.0f);
        float pulse  = 1.0f + (1.0f - std::min(tInSeg * 2.5f, 1.0f)) * 0.45f;
        float alpha  = 0.6f + (1.0f - std::min(tInSeg * 2.0f, 1.0f)) * 0.4f;

        float cH = 120.0f * pulse, cW = cH * 0.55f, gap = cW * 0.22f;
        float tw = textWidth(label, cW, gap);
        // Shadow
        drawText(quad, uiShader, proj, label,
            CX - tw/2.0f + 4, CY + cH/2.0f - 4, cW, cH, gap,
            glm::vec3(0,0,0) );
        // Teks utama
        for (auto& c : label) {
            // gambar per char supaya bisa set alpha — pakai drawText biasa
        }
        drawText(quad, uiShader, proj, label,
            CX - tw/2.0f, CY + cH/2.0f, cW, cH, gap, col);

        // Teks kecil di bawah
        if (t >= 3.0f) {
            const float sH = 14.0f, sW = sH * 0.55f, sGap = 4.0f;
            std::string sub = "GOOD  LUCK";
            float sw = textWidth(sub, sW, sGap);
            drawText(quad, uiShader, proj, sub,
                CX - sw/2.0f, CY - 30.0f, sW, sH, sGap,
                glm::vec3(0.7f, 0.95f, 0.7f));
        }
        break;
    }

    // ── GAME OVER ─────────────────────────────────────────────────
    case GameState::GAME_OVER: {
        // Overlay merah gelap
        quad.draw(uiShader, proj, 0, 0, SCR_W, SCR_H, glm::vec3(0.12f, 0, 0), 0.68f);

        // Garis merah atas & bawah
        quad.draw(uiShader, proj, 0, SCR_H - 5, SCR_W, 5, glm::vec3(0.9f,0.1f,0.1f), 0.85f);
        quad.draw(uiShader, proj, 0, 0,         SCR_W, 5, glm::vec3(0.9f,0.1f,0.1f), 0.85f);

        // Teks "GAME OVER"
        {
            const float tH = 60.0f, tW = tH * 0.55f, tGap = 8.0f;
            std::string go1 = "GAME", go2 = "OVER";
            float tw1 = textWidth(go1, tW, tGap);
            float tw2 = textWidth(go2, tW, tGap);
            // Shadow
            drawText(quad, uiShader, proj, go1, CX-tw1/2+3, CY+130.0f-3, tW, tH, tGap, glm::vec3(0.4f,0,0));
            drawText(quad, uiShader, proj, go2, CX-tw2/2+3, CY+ 62.0f-3, tW, tH, tGap, glm::vec3(0.4f,0,0));
            // Teks
            drawText(quad, uiShader, proj, go1, CX-tw1/2, CY+130.0f, tW, tH, tGap, glm::vec3(1.0f, 0.25f, 0.25f));
            drawText(quad, uiShader, proj, go2, CX-tw2/2, CY+ 62.0f, tW, tH, tGap, glm::vec3(1.0f, 0.25f, 0.25f));
        }

        // Garis pemisah
        quad.draw(uiShader, proj, CX-130, CY+42.0f, 260, 2, glm::vec3(0.6f,0.1f,0.1f), 0.7f);

        // Panel skor akhir
        {
            const float pW = 260.0f, pH = 72.0f;
            const float pX = CX - pW/2.0f, pY = CY - 10.0f;
            drawPanel(quad, uiShader, proj, pX, pY, pW, pH,
                glm::vec3(0,0,0), 0.55f, glm::vec3(1.0f,0.85f,0.2f), 0.35f);

            // Label "SCORE"
            const float lH = 11.0f, lW = lH * 0.55f, lGap = 3.0f;
            float lw = textWidth("SCORE", lW, lGap);
            drawText(quad, uiShader, proj, "SCORE",
                CX - lw/2.0f, pY + pH - 8.0f, lW, lH, lGap,
                glm::vec3(0.7f, 0.6f, 0.2f));

            // Angka skor
            const float dH = 36.0f, dW = dH * 0.55f, dGap = 8.0f;
            float nw = textWidth(std::to_string(game.score), dW, dGap) + dW;
            drawNumber(quad, uiShader, proj, game.score,
                CX + nw/2.0f, pY + pH - 22.0f,
                dW, dH, dGap, glm::vec3(1.0f, 0.95f, 0.2f));
        }

        // Panel level yang dicapai
        {
            const float lH = 13.0f, lW = lH * 0.55f, lGap = 3.5f;
            std::string lvlStr = "LEVEL  " + std::to_string(game.level) + "  REACHED";
            float lw = textWidth(lvlStr, lW, lGap);
            drawText(quad, uiShader, proj, lvlStr,
                CX - lw/2.0f, CY - 22.0f, lW, lH, lGap,
                glm::vec3(0.4f, 0.6f, 0.8f));
        }

        // Tombol PLAY AGAIN
        glm::vec3 btnCol = hover
            ? glm::vec3(0.85f, 0.25f, 0.2f)
            : glm::vec3(0.65f, 0.12f, 0.1f);
        drawButton(quad, uiShader, proj,
            btnX, btnY - 60.0f, btnW, btnH,
            btnCol, "PLAY  AGAIN", hover);
        break;
    }

    default: break;
    }
}

// ── main ──────────────────────────────────────────────────────────
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* win = glfwCreateWindow(SCR_W, SCR_H, "BrushBound", nullptr, nullptr);
    if (!win) { std::cerr << "Gagal buat window\n"; return -1; }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetKeyCallback(win, key_callback);
    glfwSetMouseButtonCallback(win, mouse_button_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Gagal init GLAD\n"; return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glViewport(0, 0, SCR_W, SCR_H);

    // ── Shader ────────────────────────────────────────────────────
    Shader    shader("shaders/basic.vert",  "shaders/basic.frag");
    Shader  uiShader("shaders/ui.vert",     "shaders/ui.frag");
    UIQuad  uiQuad;  uiQuad.init();
    glm::mat4 uiProj = glm::ortho(0.0f, (float)SCR_W, 0.0f, (float)SCR_H);

    // ── Textures ──────────────────────────────────────────────────
    unsigned int texDirt = loadTexture("textures/dirt.jpg");
    unsigned int texWall = loadTexture("textures/wall.jpg");

    // ── Arena meshes ──────────────────────────────────────────────
    float gs = (float)Game::GRID;
    MeshUV mFloor = buildFloor(gs);
    MeshUV mWall  = buildWalls(gs);

    // ── Wizard meshes (shared) ────────────────────────────────────
    Mesh mSphere   = makeSphere(0.5f, 12, 12);
    Mesh mCylinder = makeCylinder(0.5f, 0.5f, 1.0f, 14);
    Mesh mCone     = makeCone(0.5f, 1.0f, 14);

    // ── Game ──────────────────────────────────────────────────────
    Game game;
    game.init(&mSphere, &mCylinder, &mCone);

    // ── Camera ────────────────────────────────────────────────────
    glm::vec3 camPos(gs/2.5f, 5.0f, gs/2.5f - 8.0f);
    glm::vec3 camTarget(gs/2.5f, 0.5f, gs/2.5f);

    // ── Button layout (tetap sama di semua state) ─────────────────
    const float btnW = 220.0f, btnH = 64.0f;
    const float btnX = SCR_W/2.0f - btnW/2.0f;
    const float btnY = SCR_H/2.0f - btnH/2.0f;

    float lastTime = (float)glfwGetTime();
    std::cout << "BrushBound — A/D atau Left/Right: belok, ESC: keluar\n";

    while (!glfwWindowShouldClose(win)) {
        float now = (float)glfwGetTime();
        float dt  = std::min(now - lastTime, 0.05f);
        lastTime  = now;

        glfwPollEvents();

        // ── Cek klik tombol ───────────────────────────────────────
        bool hover = mouseClicked &&
            mouseX >= btnX && mouseX <= btnX + btnW &&
            (SCR_H - mouseY) >= btnY && (SCR_H - mouseY) <= btnY + btnH;

        if (hover && (game.state == GameState::MENU ||
                      game.state == GameState::GAME_OVER)) {
            game.onPlayPressed();
        }
        mouseClicked = false;

        // ── Input ke game ─────────────────────────────────────────
        if (game.state == GameState::PLAYING) {
            if (kA) game.turnLeft(dt);
            if (kD) game.turnRight(dt);
        }

        // ── Update game ───────────────────────────────────────────
        game.update(dt);

        // ── Update kamera ikuti player ────────────────────────────
        if (game.state == GameState::PLAYING || game.state == GameState::GAME_OVER) {
            const float camDist = 8.0f, camHeight = 5.0f;
            glm::vec3 pPos  = game.player.pos;
            float     pAng  = game.player.angle;
            glm::vec3 tPos(pPos.x - sinf(pAng)*camDist, pPos.y + camHeight,
                           pPos.z - cosf(pAng)*camDist);
            glm::vec3 tLook(pPos.x, pPos.y + 0.5f, pPos.z);
            float lf = 1.0f - expf(-8.0f * dt);
            camPos    = glm::mix(camPos,    tPos,  lf);
            camTarget = glm::mix(camTarget, tLook, lf);
        }

        // ════════════════════════════════════════════════════════════
        // RENDER 3D
        // ════════════════════════════════════════════════════════════
        glClearColor(0.4f, 0.65f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(glm::radians(60.0f),
            (float)SCR_W / SCR_H, 0.1f, 300.0f);
        glm::mat4 view = glm::lookAt(camPos, camTarget, glm::vec3(0,1,0));

        shader.use();
        shader.setMat4("projection", proj);
        shader.setMat4("view",       view);
        shader.setVec3("viewPos",    camPos);
        shader.setVec3("lightPos",   glm::vec3(gs/2.0f, 30.0f, gs/2.0f));
        shader.setVec3("lightColor", glm::vec3(1.0f, 0.95f, 0.85f));
        shader.setFloat("alpha",     1.0f);

        // Lantai
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texDirt);
        glUniform1i(glGetUniformLocation(shader.ID, "texSampler"), 0);
        glUniform1i(glGetUniformLocation(shader.ID, "useTexture"), 1);
        shader.setMat4("model", glm::mat4(1.0f));
        shader.setFloat("emissive", 0.0f);
        mFloor.draw();

        // Dinding
        glBindTexture(GL_TEXTURE_2D, texWall);
        shader.setMat4("model", glm::mat4(1.0f));
        shader.setFloat("emissive", 0.0f);
        mWall.draw();

        // Wizard player + trail
        glUniform1i(glGetUniformLocation(shader.ID, "useTexture"), 0);
        renderWizard(game.player, shader, mCylinder);

        // Bot-bot + trail mereka
        for (Wizard* w : game.botWizards())
            renderWizard(*w, shader, mCylinder);

        // ════════════════════════════════════════════════════════════
        // RENDER UI
        // ════════════════════════════════════════════════════════════
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        uiShader.use();

        renderUI(game, uiQuad, uiShader, uiProj, btnX, btnY, btnW, btnH, hover);

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        glfwSwapBuffers(win);
    }

    glfwTerminate();
    return 0;
}
#pragma once
// ── UI.h ─────────────────────────────────────────────────────────
// Helper sederhana untuk render UI 2D (HUD skor, tombol Play/Restart)
// tanpa perlu font file eksternal. Angka digambar gaya "7-segment"
// memakai kotak-kotak kecil (segmen horizontal & vertikal).
// ────────────────────────────────────────────────────────────────
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

#include "Shader.h"

// Quad 1x1 (0..1 di X dan Y) yang akan di-scale & translate lewat model matrix
class UIQuad {
public:
  unsigned int VAO = 0, VBO = 0;

  void init() {
    float verts[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                     0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
  }

  // Gambar rect di screen space: posisi (x,y) = pojok kiri-bawah dalam pixel,
  // lebar w, tinggi h, warna color, alpha opacity.
  void draw(Shader &uiShader, const glm::mat4 &projection, float x, float y,
            float w, float h, glm::vec3 color, float alpha = 1.0f) const {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));
    uiShader.setMat4("model", model);
    uiShader.setMat4("projection", projection);
    uiShader.setVec3("color", color);
    uiShader.setFloat("alpha", alpha);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
  }
};

// ── 7-segment digit definitions ───────────────────────────────────
// Tiap digit 0-9 direpresentasikan dengan 7 segmen (a,b,c,d,e,f,g):
//   _a_
//  f   b
//   _g_
//  e   c
//   _d_
// segMask bit order: a,b,c,d,e,f,g (bit0=a ... bit6=g)
static const unsigned char DIGIT_SEGMENTS[10] = {
    0b0111111, // 0: a b c d e f
    0b0000110, // 1: b c
    0b1011011, // 2: a b d e g
    0b1001111, // 3: a b c d g
    0b1100110, // 4: b c f g
    0b1101101, // 5: a c d f g
    0b1111101, // 6: a c d e f g
    0b0000111, // 7: a b c
    0b1111111, // 8: all
    0b1101111  // 9: a b c d f g
};

// Gambar satu digit di posisi (x,y) pojok kiri-bawah, dengan ukuran (w,h).
inline void drawDigit(UIQuad &quad, Shader &uiShader, const glm::mat4 &proj,
                      int digit, float x, float y, float w, float h,
                      glm::vec3 color) {
  if (digit < 0 || digit > 9)
    return;
  unsigned char mask = DIGIT_SEGMENTS[digit];
  float t = w * 0.22f;
  float halfH = h * 0.5f;

  if (mask & 0b0000001)
    quad.draw(uiShader, proj, x + t, y + h - t, w - 2 * t, t, color);
  if (mask & 0b0000010)
    quad.draw(uiShader, proj, x + w - t, y + halfH, t, halfH - t * 0.5f, color);
  if (mask & 0b0000100)
    quad.draw(uiShader, proj, x + w - t, y, t, halfH - t * 0.5f, color);
  if (mask & 0b0001000)
    quad.draw(uiShader, proj, x + t, y, w - 2 * t, t, color);
  if (mask & 0b0010000)
    quad.draw(uiShader, proj, x, y, t, halfH - t * 0.5f, color);
  if (mask & 0b0100000)
    quad.draw(uiShader, proj, x, y + halfH, t, halfH - t * 0.5f, color);
  if (mask & 0b1000000)
    quad.draw(uiShader, proj, x + t, y + halfH - t * 0.5f, w - 2 * t, t, color);
}

inline void drawNumber(UIQuad &quad, Shader &uiShader, const glm::mat4 &proj,
                       int number, float xTopLeft, float yTopLeft, float digitW,
                       float digitH, float gap, glm::vec3 color) {
  bool negative = number < 0;
  if (negative)
    number = -number;

  std::string s = std::to_string(number);
  float totalW = s.size() * digitW + (s.size() - 1) * gap;
  if (negative)
    totalW += digitW * 0.5f + gap;

  float startX = xTopLeft - totalW;
  float y = yTopLeft - digitH;

  float cx = startX;
  if (negative) {
    quad.draw(uiShader, proj, cx, y + digitH * 0.5f - digitH * 0.07f,
              digitW * 0.5f, digitH * 0.14f, color);
    cx += digitW * 0.5f + gap;
  }
  for (char c : s) {
    int d = c - '0';
    drawDigit(quad, uiShader, proj, d, cx, y, digitW, digitH, color);
    cx += digitW + gap;
  }
}

// ── 7-segment LETTER definitions ──────────────────────────────────
inline unsigned char letterSegmentMask(char c) {
  switch (c) {
  case 'L': return 0b0110000;
  case 'E': return 0b1111001;
  case 'V': return 0b0111000;
  case 'O': return 0b0111111;
  case 'G': return 0b1111101;
  case 'S': return 0b1101101;
  case 'T': return 0b1110000;
  case 'A': return 0b1110111;
  case 'P': return 0b1110011;
  case 'U': return 0b0111110;
  default:  return 0b0000000;
  }
}

inline void drawChar(UIQuad &quad, Shader &uiShader, const glm::mat4 &proj,
                     char c, float x, float y, float w, float h,
                     glm::vec3 color) {
  if (c == ' ') return;
  if (c == '-') {
    float t = h * 0.16f;
    quad.draw(uiShader, proj, x, y + h * 0.5f - t * 0.5f, w, t, color);
    return;
  }
  if (c >= '0' && c <= '9') {
    drawDigit(quad, uiShader, proj, c - '0', x, y, w, h, color);
    return;
  }
  unsigned char mask = letterSegmentMask(c);
  if (mask == 0) return;

  float t = w * 0.22f;
  float halfH = h * 0.5f;
  if (mask & 0b0000001)
    quad.draw(uiShader, proj, x + t, y + h - t, w - 2 * t, t, color);
  if (mask & 0b0000010)
    quad.draw(uiShader, proj, x + w - t, y + halfH, t, halfH - t * 0.5f, color);
  if (mask & 0b0000100)
    quad.draw(uiShader, proj, x + w - t, y, t, halfH - t * 0.5f, color);
  if (mask & 0b0001000)
    quad.draw(uiShader, proj, x + t, y, w - 2 * t, t, color);
  if (mask & 0b0010000)
    quad.draw(uiShader, proj, x, y, t, halfH - t * 0.5f, color);
  if (mask & 0b0100000)
    quad.draw(uiShader, proj, x, y + halfH, t, halfH - t * 0.5f, color);
  if (mask & 0b1000000)
    quad.draw(uiShader, proj, x + t, y + halfH - t * 0.5f, w - 2 * t, t, color);
}

inline void drawText(UIQuad &quad, Shader &uiShader, const glm::mat4 &proj,
                     const std::string &text, float xLeft, float yTopLeft,
                     float charW, float charH, float gap, glm::vec3 color) {
  float y = yTopLeft - charH;
  float cx = xLeft;
  for (char c : text) {
    drawChar(quad, uiShader, proj, c, cx, y, charW, charH, color);
    cx += charW + gap;
  }
}

inline float textWidth(const std::string &text, float charW, float gap) {
  if (text.empty()) return 0.0f;
  return text.size() * charW + (text.size() - 1) * gap;
}

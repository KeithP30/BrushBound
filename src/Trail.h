#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>
#include "Shader.h"
#include "Mesh.h"

// ── Trail.h ──────────────────────────────────────────────────────
// Mengelola trail (jejak) continuous untuk satu wizard.
// Memisahkan logika trail dari Wizard agar keduanya bisa dipakai
// secara independen.
// ─────────────────────────────────────────────────────────────────

struct TrailSegment {
    glm::vec3 pos;
    glm::vec3 color;
    float lifetime;
    float age;
};

class Trail {
public:
    std::vector<TrailSegment> segments;
    glm::vec3 color;

    static const int MAX_SEGMENTS  = 80;
    float interval   = 0.08f; // lebih rapat = dinding lebih mulus
    float timer      = 0.0f;

    // Dimensi dinding Tron
    static constexpr float WALL_HEIGHT = 0.9f;  // lebih pendek dari wizard
    static constexpr float WALL_WIDTH  = 0.12f; // ketebalan dinding

    Trail() = default;
    explicit Trail(glm::vec3 col) : color(col) {}

    void clear() {
        segments.clear();
        timer = 0.0f;
    }

    // Dipanggil tiap frame — tambah titik baru jika waktunya
    void update(float dt, const glm::vec3& pos, bool emitting) {
        // Aging & expire
        for (auto& s : segments) s.age += dt;
        segments.erase(
            std::remove_if(segments.begin(), segments.end(),
                [](const TrailSegment& s){ return s.age >= s.lifetime; }),
            segments.end()
        );

        // Batasi panjang
        if ((int)segments.size() > MAX_SEGMENTS)
            segments.erase(segments.begin(),
                segments.begin() + segments.size() - MAX_SEGMENTS);

        // Tambah titik baru
        if (!emitting) return;
        timer += dt;
        if (timer >= interval) {
            timer = 0.0f;
            segments.push_back({ pos, color, 4.0f, 0.0f });
        }
    }

    // Cek apakah pos menabrak trail ini
    bool checkCollision(const glm::vec3& pos, float radius = 0.4f) const {
        if (segments.size() < 2) return false;
        for (size_t i = 1; i < segments.size(); i++) {
            if (distToSegment(pos, segments[i-1].pos, segments[i].pos) < radius)
                return true;
        }
        return false;
    }

    // Render trail sebagai dinding vertikal gaya Tron
    void draw(Shader& shader, Mesh& mCylinder) const {
        if (segments.size() < 2) return;
        for (size_t i = 1; i < segments.size(); i++) {
            glm::vec3 p1  = segments[i-1].pos;
            glm::vec3 p2  = segments[i].pos;
            glm::vec3 dir = p2 - p1;
            float dist = glm::length(dir);
            if (dist < 0.001f) continue;
            dir = glm::normalize(dir);

            // Tengah segmen, dinaikkan setengah tinggi dinding
            glm::vec3 mid = (p1 + p2) * 0.5f + glm::vec3(0, WALL_HEIGHT * 0.5f, 0);
            glm::mat4 m   = glm::translate(glm::mat4(1), mid);

            // Orientasi: dir = panjang dinding, Y = vertikal, right = tipis
            glm::vec3 up    = glm::vec3(0, 1, 0);
            glm::vec3 right = glm::cross(dir, up);
            if (glm::length(right) < 0.001f) right = glm::vec3(1, 0, 0);
            right = glm::normalize(right);

            // Scale: X = tipis (WALL_WIDTH), Y = tinggi, Z = panjang segmen
            m = m * glm::mat4(glm::vec4(right, 0), glm::vec4(up, 0),
                              glm::vec4(dir,   0), glm::vec4(0, 0, 0, 1));
            m = glm::scale(m, glm::vec3(WALL_WIDTH, WALL_HEIGHT, dist));

            // Fade opacity di ujung lifetime
            float fade = 1.0f - (segments[i].age / segments[i].lifetime);
            shader.setMat4("model", m);
            shader.setVec3("objectColor", segments[i].color);
            shader.setFloat("emissive", 0.7f * fade);
            shader.setFloat("alpha", fade);
            mCylinder.draw();
        }
        shader.setFloat("alpha", 1.0f);
    }

private:
    static float distToSegment(const glm::vec3& p,
                                const glm::vec3& a,
                                const glm::vec3& b) {
        glm::vec3 pa = p - a, ba = b - a;
        float h = glm::clamp(glm::dot(pa, ba) / glm::dot(ba, ba), 0.0f, 1.0f);
        return glm::length(pa - ba * h);
    }
};
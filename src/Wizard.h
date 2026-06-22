#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "Shader.h"
#include "Mesh.h"
#include "Trail.h"

// ── Wizard.h ─────────────────────────────────────────────────────
// Merepresentasikan satu wizard (player atau bot) dalam mode
// continuous free-roam. Mengelola posisi, sudut, animasi bob,
// trail, dan efek ledakan partikel.
// ─────────────────────────────────────────────────────────────────

class Wizard {
public:
    // ── State ────────────────────────────────────────────────────
    glm::vec3 pos;
    float     angle     = 0.0f;   // radian, arah hadap
    float     speed     = 14.0f;
    bool      alive     = true;

    Trail     trail;

    // Warna visual
    glm::vec3 bodyColor;
    glm::vec3 hatColor;
    glm::vec3 robeColor;

    // ── Explosion particles ──────────────────────────────────────
    struct Particle {
        glm::vec3 pos, vel;
        float life;
    };
    std::vector<Particle> particles;
    bool exploding = false;

    // ── Meshes (shared, tidak dimiliki wizard) ───────────────────
    Mesh* mSphere   = nullptr;
    Mesh* mCylinder = nullptr;
    Mesh* mCone     = nullptr;

    // ── Animasi ──────────────────────────────────────────────────
    float bobTime = 0.0f;

    // ── Constructor ──────────────────────────────────────────────
    Wizard() = default;

    Wizard(glm::vec3 startPos, float startAngle,
           glm::vec3 body, glm::vec3 hat, glm::vec3 robe,
           glm::vec3 trailCol,
           Mesh* sphere, Mesh* cyl, Mesh* cone)
        : pos(startPos), angle(startAngle)
        , bodyColor(body), hatColor(hat)
        , trail(trailCol)
        , robeColor(robe)
        , mSphere(sphere), mCylinder(cyl), mCone(cone)
    {}

    // ── Reset ke posisi baru (untuk respawn) ─────────────────────
    void reset(glm::vec3 newPos, float newAngle) {
        pos        = newPos;
        angle      = newAngle;
        alive      = true;
        exploding  = false;
        bobTime    = 0.0f;
        particles.clear();
        trail.clear();
    }

    // ── Update animasi & partikel ────────────────────────────────
    void update(float dt) {
        bobTime += dt;

        if (exploding) {
            for (auto& p : particles) {
                p.pos   += p.vel * dt;
                p.vel.y -= 4.0f * dt;   // gravitasi
                p.life  -= dt;
            }
            particles.erase(
                std::remove_if(particles.begin(), particles.end(),
                    [](const Particle& p){ return p.life <= 0.0f; }),
                particles.end()
            );
            if (particles.empty()) exploding = false;
        }
    }

    // ── Trigger efek ledakan partikel ────────────────────────────
    void triggerExplosion() {
        alive     = false;
        exploding = true;
        particles.clear();

        for (int i = 0; i < 40; i++) {
            float a     = (float)i / 40.0f * 6.28318f;
            float spd   = 2.0f + (rand() % 300) / 100.0f;
            float vy    = 1.0f + (rand() % 300) / 150.0f;
            Particle p;
            p.pos  = pos + glm::vec3(0, 0.5f, 0);
            p.vel  = glm::vec3(cosf(a) * spd, vy, sinf(a) * spd);
            p.life = 1.2f + (rand() % 100) / 200.0f;
            particles.push_back(p);
        }
    }

    // ── Render wizard + partikel ─────────────────────────────────
    void draw(Shader& shader) const {
        if (!alive && !exploding) return;
        if (!mSphere || !mCylinder || !mCone) return; // mesh belum diinit

        float     bob  = sinf(bobTime * 3.0f) * 0.08f;
        glm::vec3 base = pos + glm::vec3(0.0f, 0.5f + bob, 0.0f);
        glm::mat4 rot  = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0,1,0));

        shader.setFloat("alpha", 1.0f);

        if (alive) {
            // Kepala
            {
                glm::mat4 m = glm::translate(glm::mat4(1.0f), base + glm::vec3(0, 0.55f, 0));
                m = glm::scale(m, glm::vec3(0.28f));
                shader.setMat4("model", m);
                shader.setVec3("objectColor", bodyColor);
                shader.setFloat("emissive", 0.0f);
                mSphere->draw();
            }
            // Topi
            {
                glm::mat4 m = glm::translate(glm::mat4(1.0f), base + glm::vec3(0, 0.85f, 0));
                m = glm::scale(m, glm::vec3(0.22f, 0.45f, 0.22f));
                shader.setMat4("model", m);
                shader.setVec3("objectColor", hatColor);
                shader.setFloat("emissive", 0.3f);
                mCone->draw();
            }
            // Jubah
            {
                glm::mat4 m = glm::translate(glm::mat4(1.0f), base + glm::vec3(0, 0.1f, 0));
                m = m * rot;
                m = glm::scale(m, glm::vec3(0.38f, 0.55f, 0.38f));
                shader.setMat4("model", m);
                shader.setVec3("objectColor", robeColor);
                shader.setFloat("emissive", 0.1f);
                mCylinder->draw();
            }
            // Gagang sapu
            {
                glm::mat4 m = glm::translate(glm::mat4(1.0f), base + glm::vec3(0, -0.10f, 0));
                m = m * rot;
                m = glm::rotate(m, glm::radians(90.0f), glm::vec3(1,0,0));
                m = glm::scale(m, glm::vec3(0.06f, 1.1f, 0.06f));
                shader.setMat4("model", m);
                shader.setVec3("objectColor", glm::vec3(0.6f, 0.4f, 0.1f));
                shader.setFloat("emissive", 0.0f);
                mCylinder->draw();
            }
            // Bulu sapu
            {
                glm::mat4 m = glm::translate(glm::mat4(1.0f), base + glm::vec3(0, -0.10f, 0));
                m = m * rot;
                m = glm::translate(m, glm::vec3(0, 0, -0.58f));
                m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1,0,0));
                m = glm::scale(m, glm::vec3(0.20f, 0.30f, 0.20f));
                shader.setMat4("model", m);
                shader.setVec3("objectColor", glm::vec3(0.5f, 0.35f, 0.08f));
                shader.setFloat("emissive", 0.0f);
                mCone->draw();
            }
            // Shadow
            {
                glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, 0.02f, pos.z));
                m = glm::scale(m, glm::vec3(0.5f, 0.01f, 0.5f));
                shader.setMat4("model", m);
                shader.setVec3("objectColor", glm::vec3(0, 0, 0));
                shader.setFloat("emissive", 0.0f);
                mCylinder->draw();
            }
        }

        // Partikel ledakan
        if (exploding) {
            shader.setFloat("emissive", 0.8f);
            for (auto& p : particles) {
                float t = 1.0f - p.life / 1.2f;
                shader.setVec3("objectColor", trail.color * (1.0f - t * 0.5f));
                shader.setFloat("alpha", p.life / 1.2f);
                glm::mat4 m = glm::translate(glm::mat4(1.0f), p.pos);
                float s = 0.12f * (1.0f - t * 0.6f);
                m = glm::scale(m, glm::vec3(s));
                shader.setMat4("model", m);
                mSphere->draw();
            }
            shader.setFloat("alpha", 1.0f);
        }
    }

    bool isDone() const { return !alive && !exploding; }
};

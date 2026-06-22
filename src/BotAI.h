#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>
#include "Wizard.h"
#include "Trail.h"

// ── BotAI.h ──────────────────────────────────────────────────────
// Logika AI untuk bot musuh dalam mode continuous free-roam.
// Dipanggil tiap frame untuk mengupdate angle dan posisi bot.
// ─────────────────────────────────────────────────────────────────

struct BotState {
    float turnTimer    = 0.0f;
    float turnInterval = 2.0f;  // detik antar variasi random
    float spawnTimer   = 0.0f;
    float spawnInterval = 1.5f; // detik tunggu sebelum respawn
    bool  waitingRespawn = false;
};

class BotAI {
public:
    // ── Satu frame update untuk satu bot ─────────────────────────
    // Mengupdate wizard.angle dan wizard.pos.
    // Mengembalikan true jika bot kena trail player (→ harus mati).
    static bool update(Wizard& bot, BotState& state,
                       const glm::vec3& playerPos,
                       const Trail& playerTrail,
                       const std::vector<Wizard*>& allBots,  // untuk separation
                       float dt, int gridSize)
    {
        if (!bot.alive) return false;

        // ── Arah ke player ────────────────────────────────────────
        glm::vec3 dirToPlayer = glm::normalize(playerPos - bot.pos);

        // ── Separation: dorongan menjauh dari bot lain ────────────
        glm::vec3 separation(0.0f);
        const float SEP_RADIUS = 8.0f;
        for (Wizard* other : allBots) {
            if (other == &bot || !other->alive) continue;
            glm::vec3 diff = bot.pos - other->pos;
            float dist = glm::length(diff);
            if (dist > 0.001f && dist < SEP_RADIUS)
                separation += glm::normalize(diff) * (SEP_RADIUS - dist) / SEP_RADIUS;
        }

        glm::vec3 desired = dirToPlayer + separation * 1.8f;
        if (glm::length(desired) < 0.001f) desired = dirToPlayer;
        desired = glm::normalize(desired);

        float targetAngle = atan2f(desired.x, desired.z);

        // ── Variasi random sesekali ───────────────────────────────
        state.turnTimer += dt;
        if (state.turnTimer >= state.turnInterval) {
            state.turnTimer = 0.0f;
            if (rand() % 100 < 30)
                targetAngle += (rand() % 61 - 30) * 0.01745329f;
        }

        // ── Trail & obstacle avoidance ────────────────────────────
        float detectDist = 6.0f;
        float turnSpeed  = 2.0f;

        bool currentSafe = isSafe(bot.pos, bot.angle,     detectDist, playerTrail, gridSize);
        bool targetSafe  = isSafe(bot.pos, targetAngle,   detectDist, playerTrail, gridSize);

        if (!currentSafe || !targetSafe) {
            turnSpeed = 7.0f;
            float best = bot.angle;
            bool found = scanForSafe(bot.pos, bot.angle, detectDist, playerTrail, gridSize, best);
            if (!found) {
                // coba jarak lebih pendek
                scanForSafe(bot.pos, bot.angle, 3.0f, playerTrail, gridSize, best);
            }
            if (found) targetAngle = best;
        }

        // ── Putar ke targetAngle ──────────────────────────────────
        float diff = targetAngle - bot.angle;
        while (diff >  3.14159f) diff -= 6.28318f;
        while (diff < -3.14159f) diff += 6.28318f;
        if      (diff >  0.05f) bot.angle += turnSpeed * dt;
        else if (diff < -0.05f) bot.angle -= turnSpeed * dt;

        // ── Gerak maju ────────────────────────────────────────────
        float margin = 1.0f;
        glm::vec3 dir    = glm::vec3(sinf(bot.angle), 0, cosf(bot.angle));
        glm::vec3 newPos = bot.pos + dir * bot.speed * dt;

        bool hitWall = (newPos.x <= margin || newPos.x >= (float)gridSize - 1.0f - margin ||
                        newPos.z <= margin || newPos.z >= (float)gridSize - 1.0f - margin);

        newPos.x = glm::clamp(newPos.x, margin, (float)gridSize - 1.0f - margin);
        newPos.z = glm::clamp(newPos.z, margin, (float)gridSize - 1.0f - margin);

        // ── Cek tabrakan dengan trail player ─────────────────────
        if (playerTrail.checkCollision(newPos)) return true;  // → bot mati

        if (!hitWall) {
            bot.pos = newPos;
        } else {
            bot.angle += (rand() % 180 - 90) * 0.01745329f;
        }

        return false;
    }

    // ── Update timer respawn, kembalikan true saat siap spawn ────
    static bool updateRespawn(BotState& state, float dt) {
        state.spawnTimer += dt;
        if (state.spawnTimer >= state.spawnInterval) {
            state.spawnTimer = 0.0f;
            return true;
        }
        return false;
    }

private:
    // Cek apakah arah `angle` dari `pos` aman sejauh `dist`
    static bool isSafe(const glm::vec3& pos, float angle, float dist,
                       const Trail& playerTrail, int gridSize) {
        glm::vec3 dir(sinf(angle), 0, cosf(angle));
        float margin = 1.5f;
        for (float d = 0.5f; d <= dist; d += 0.5f) {
            glm::vec3 tp = pos + dir * d;
            if (tp.x <= margin || tp.x >= (float)gridSize - 1.0f - margin ||
                tp.z <= margin || tp.z >= (float)gridSize - 1.0f - margin)
                return false;
            if (playerTrail.checkCollision(tp)) return false;
        }
        return true;
    }

    // Scan kiri/kanan untuk sudut yang aman, isi `best`, kembalikan true jika ketemu
    static bool scanForSafe(const glm::vec3& pos, float curAngle, float dist,
                             const Trail& playerTrail, int gridSize, float& best) {
        for (float offset = 0.26f; offset <= 3.14f; offset += 0.26f) {
            if (isSafe(pos, curAngle + offset, dist, playerTrail, gridSize)) {
                best = curAngle + offset; return true;
            }
            if (isSafe(pos, curAngle - offset, dist, playerTrail, gridSize)) {
                best = curAngle - offset; return true;
            }
        }
        return false;
    }
};

#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Wizard.h"
#include "BotAI.h"

// ── Game.h ───────────────────────────────────────────────────────
// Mengelola state machine, scoring, level, dan daftar bot.
// main.cpp hanya perlu memanggil update() tiap frame dan
// membaca state untuk render.
// ─────────────────────────────────────────────────────────────────

enum class GameState { MENU, COUNTDOWN, PLAYING, GAME_OVER };

// ── Konfigurasi level ─────────────────────────────────────────────
static constexpr int LEVEL_COUNT = 10;
static constexpr int LEVEL_SCORE_THRESHOLD[LEVEL_COUNT] = {
    200, 400, 600, 800, 1000, 1200, 1400, 1600, 1800, 2000
};
static constexpr int LEVEL_BOT_COUNT[LEVEL_COUNT] = {
    1, 2, 2, 3, 3, 4, 4, 5, 5, 6
};

static int botCountForLevel(int level) {
    level = std::max(1, std::min(level, LEVEL_COUNT));
    return LEVEL_BOT_COUNT[level - 1];
}

static int computeLevel(int score) {
    int level = 1;
    for (int i = 0; i < LEVEL_COUNT - 1; i++) {
        if (score >= LEVEL_SCORE_THRESHOLD[i]) level = i + 2;
        else break;
    }
    return std::min(level, LEVEL_COUNT);
}

// ── Satu slot bot (wizard + AI state) ────────────────────────────
struct BotSlot {
    Wizard   wizard;
    BotState aiState;
};

// ── Game ─────────────────────────────────────────────────────────
class Game {
public:
    // ── Konstan ──────────────────────────────────────────────────
    static constexpr float COUNTDOWN_DURATION  = 3.99f;
    static constexpr float SURVIVAL_TICK       = 1.0f;
    static constexpr int   SURVIVAL_SCORE      = 10;
    static constexpr int   BOT_KILL_SCORE      = 100;
    static constexpr int   GRID                = 120;

    // ── State yang dibaca main.cpp ────────────────────────────────
    GameState state      = GameState::MENU;
    int       score      = 0;
    int       level      = 1;
    float     countdown  = 0.0f;   // waktu berjalan sejak countdown mulai

    Wizard              player;
    std::vector<BotSlot> bots;

    // ── Init: panggil sekali setelah semua mesh siap ──────────────
    void init(Mesh* sphere, Mesh* cyl, Mesh* cone) {
        mSphere   = sphere;
        mCylinder = cyl;
        mCone     = cone;
        reset(); // inisialisasi player & bot setelah pointer mesh valid
    }

    // ── Mulai / restart game ─────────────────────────────────────
    void reset() {
        score    = 0;
        level    = 1;
        countdown = 0.0f;
        survivalTimer  = 0.0f;
        lastLoggedScore = -1;

        // Reset player
        glm::vec3 pStart((float)GRID / 2.5f, 0.0f, (float)GRID / 2.5f);
        player = makeWizard(pStart, 0.0f,
            glm::vec3(0.95f, 0.85f, 0.75f),  // kepala (skin)
            glm::vec3(0.2f,  0.0f,  0.8f),   // topi (ungu)
            glm::vec3(0.15f, 0.0f,  0.55f),  // jubah (ungu gelap)
            glm::vec3(0.2f,  0.0f,  0.8f)    // trail (biru)
        );
        player.speed = 14.0f;

        // Spawn bot pertama
        bots.clear();
        BotSlot first;
        first.wizard = makeWizard(
            glm::vec3((float)GRID / 2.0f, 0.0f, (float)GRID / 1.5f), 0.0f,
            glm::vec3(1.0f, 0.7f, 0.7f),   // kepala (merah muda)
            glm::vec3(0.8f, 0.0f, 0.0f),   // topi (merah)
            glm::vec3(0.7f, 0.0f, 0.0f),   // jubah (merah gelap)
            glm::vec3(0.8f, 0.0f, 0.0f)    // trail (merah)
        );
        first.wizard.speed = 12.0f;
        first.aiState = BotState{};
        bots.push_back(std::move(first));
    }

    // ── Klik tombol Play / Restart ────────────────────────────────
    void onPlayPressed() {
        reset();
        state = GameState::COUNTDOWN;
    }

    // ── Update satu frame — kembalikan true jika state berubah ───
    void update(float dt) {
        switch (state) {
        case GameState::COUNTDOWN: updateCountdown(dt); break;
        case GameState::PLAYING:   updatePlaying(dt);   break;
        default: break;
        }
    }

    // ── Kontrol player (dipanggil dari key callback) ──────────────
    void turnLeft(float dt)  { if (player.alive) player.angle += 2.5f * dt; }
    void turnRight(float dt) { if (player.alive) player.angle -= 2.5f * dt; }

    // ── Pointer ke semua wizard (untuk render di main) ───────────
    // Kembalikan list pointer agar main bisa iterate tanpa tahu struktur internal
    std::vector<Wizard*> botWizards() {
        std::vector<Wizard*> out;
        for (auto& b : bots) out.push_back(&b.wizard);
        return out;
    }

private:
    // ── Internal ─────────────────────────────────────────────────
    Mesh* mSphere   = nullptr;
    Mesh* mCylinder = nullptr;
    Mesh* mCone     = nullptr;

    float survivalTimer   = 0.0f;
    int   lastLoggedScore = -1;

    // ── Factory wizard ────────────────────────────────────────────
    Wizard makeWizard(glm::vec3 pos, float angle,
                      glm::vec3 body, glm::vec3 hat, glm::vec3 robe,
                      glm::vec3 trailCol) {
        return Wizard(pos, angle, body, hat, robe, trailCol,
                      mSphere, mCylinder, mCone);
    }

    // ── Spawn bot acak ────────────────────────────────────────────
    BotSlot spawnBot() {
        BotSlot slot;
        float rx = 5.0f + (rand() % (GRID - 10));
        float rz = 5.0f + (rand() % (GRID - 10));
        slot.wizard = makeWizard(
            glm::vec3(rx, 0.0f, rz),
            (float)(rand() % 360) * 0.01745329f,
            glm::vec3(1.0f, 0.7f, 0.7f),
            glm::vec3(0.8f, 0.0f, 0.0f),
            glm::vec3(0.7f, 0.0f, 0.0f),
            glm::vec3(0.8f, 0.0f, 0.0f)
        );
        slot.wizard.speed = 12.0f;
        slot.aiState.turnInterval = 1.5f + (float)(rand() % 100) / 100.0f;
        slot.aiState.turnTimer    = (float)(rand() % 100) / 100.0f * slot.aiState.turnInterval;
        return slot;
    }

    // ── Tambah bot sampai target level ───────────────────────────
    void ensureBotCount() {
        int target = botCountForLevel(level);
        while ((int)bots.size() < target) {
            bots.push_back(spawnBot());
            std::cout << "Bot baru! Total target level " << level
                      << ": " << target << "\n";
        }
    }

    // ── Cek & naikkan level ───────────────────────────────────────
    void checkLevelUp() {
        int newLevel = computeLevel(score);
        if (newLevel != level) {
            level = newLevel;
            std::cout << "Naik ke Level " << level << "! Skor: " << score << "\n";
            ensureBotCount();
        }
    }

    // ── Update countdown ──────────────────────────────────────────
    void updateCountdown(float dt) {
        countdown += dt;
        if (countdown >= COUNTDOWN_DURATION)
            state = GameState::PLAYING;
    }

    // ── Update satu frame gameplay ────────────────────────────────
    void updatePlaying(float dt) {
        if (!player.alive) return;

        // ── Gerak player ──────────────────────────────────────────
        glm::vec3 dir    = glm::vec3(sinf(player.angle), 0, cosf(player.angle));
        glm::vec3 newPos = player.pos + dir * player.speed * dt;
        float margin     = 1.0f;

        bool hitWall = (newPos.x <= margin || newPos.x >= (float)GRID - 1.0f - margin ||
                        newPos.z <= margin || newPos.z >= (float)GRID - 1.0f - margin);

        newPos.x = glm::clamp(newPos.x, margin, (float)GRID - 1.0f - margin);
        newPos.z = glm::clamp(newPos.z, margin, (float)GRID - 1.0f - margin);

        // Cek tabrakan trail bot
        bool hitBotTrail = false;
        for (auto& b : bots) {
            if (b.wizard.trail.checkCollision(newPos)) {
                hitBotTrail = true; break;
            }
        }

        if (hitWall || hitBotTrail) {
            player.triggerExplosion();
            state = GameState::GAME_OVER;
            std::cout << "Game Over! Skor: " << score << "\n";
            return;
        }

        player.pos = newPos;

        // Update trail player
        player.trail.update(dt, player.pos, true);

        // Survival scoring
        survivalTimer += dt;
        if (survivalTimer >= SURVIVAL_TICK) {
            survivalTimer -= SURVIVAL_TICK;
            score += SURVIVAL_SCORE;
            if (score != lastLoggedScore) {
                std::cout << "Skor: " << score << "\n";
                lastLoggedScore = score;
            }
        }

        checkLevelUp();

        // ── Update bot ────────────────────────────────────────────
        auto botPtrs = botWizards();
        for (auto& b : bots) {
            if (b.wizard.alive) {
                // Update trail bot
                b.wizard.trail.update(dt, b.wizard.pos, true);

                // Jalankan AI
                bool killed = BotAI::update(
                    b.wizard, b.aiState,
                    player.pos, player.trail,
                    botPtrs, dt, GRID
                );

                if (killed) {
                    b.wizard.trail.clear();
                    b.wizard.triggerExplosion();
                    score += BOT_KILL_SCORE;
                    std::cout << "Bot mati! +" << BOT_KILL_SCORE
                              << " skor. Total: " << score << "\n";
                    checkLevelUp();
                }
            } else {
                // Update partikel ledakan
                b.wizard.update(dt);

                // Respawn jika partikel sudah selesai
                if (b.wizard.isDone()) {
                    if (BotAI::updateRespawn(b.aiState, dt)) {
                        float rx = 5.0f + (rand() % (GRID - 10));
                        float rz = 5.0f + (rand() % (GRID - 10));
                        b.wizard.reset(glm::vec3(rx, 0.0f, rz), 0.0f);
                        b.aiState.turnTimer  = 0.0f;
                        b.aiState.waitingRespawn = false;
                    }
                }
            }
        }

        // Update animasi player
        player.update(dt);
    }
};
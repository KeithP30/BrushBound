#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include "Mesh.h"
#include <vector>

struct TrailCell {
    int x, z; // grid position
};

class Wizard {
public:
    // Grid position & direction
    int gx, gz;
    int dx, dz;       // current direction
    int ndx, ndz;     // next direction (buffered)

    glm::vec3 color;
    glm::vec3 trailColor;

    // World position (smooth)
    glm::vec3 worldPos;
    float angle;      // facing angle (radians)

    // Trail
    std::vector<TrailCell> trail;

    // Meshes (shared, passed in)
    Mesh* mSphere;
    Mesh* mCylinder;
    Mesh* mCone;

    // Bob animation
    float bobTime;
    bool alive;

    // Explosion particles
    struct Particle { glm::vec3 pos, vel; float life; };
    std::vector<Particle> particles;
    bool exploding;

    Wizard(int startX, int startZ, int dirX, int dirZ,
           glm::vec3 col, glm::vec3 trailCol,
           Mesh* sphere, Mesh* cyl, Mesh* cone)
        : gx(startX), gz(startZ), dx(dirX), dz(dirZ),
          ndx(dirX), ndz(dirZ),
          color(col), trailColor(trailCol),
          mSphere(sphere), mCylinder(cyl), mCone(cone),
          bobTime(0.0f), alive(true), exploding(false)
    {
        worldPos = gridToWorld(gx, gz);
        angle = atan2f((float)dx, (float)dz);
    }

    static glm::vec3 gridToWorld(int x, int z) {
        return glm::vec3((float)x, 0.0f, (float)z);
    }

    void setDirection(int newDx, int newDz) {
        // Cannot reverse direction
        if (newDx == -dx && newDz == -dz) return;
        ndx = newDx; ndz = newDz;
    }

    // Advance one grid step, returns false if collision
    bool step(const std::vector<TrailCell>& otherTrail, int gridSize) {
        dx = ndx; dz = ndz;
        int nx = gx + dx;
        int nz = gz + dz;

        // Wall collision
        if (nx < 0 || nx >= gridSize || nz < 0 || nz >= gridSize)
            return false;

        // Self collision
        for (auto& t : trail)
            if (t.x == nx && t.z == nz) return false;

        // Other trail collision
        for (auto& t : otherTrail)
            if (t.x == nx && t.z == nz) return false;

        trail.push_back({gx, gz});
        gx = nx; gz = nz;
        worldPos = gridToWorld(gx, gz);
        angle = atan2f((float)dx, (float)dz);
        return true;
    }

    void update(float dt) {
        bobTime += dt;

        if (exploding) {
            for (auto& p : particles) {
                p.pos += p.vel * dt;
                p.vel.y -= 4.0f * dt;
                p.life -= dt;
            }
            particles.erase(
                std::remove_if(particles.begin(), particles.end(),
                    [](const Particle& p){ return p.life <= 0; }),
                particles.end()
            );
        }
    }

    void triggerExplosion() {
        exploding = true;
        alive = false;
        // Spawn paint splat particles
        for (int i = 0; i < 40; i++) {
            float a = (float)i / 40.0f * 6.28318f;
            float speed = 2.0f + (rand() % 300) / 100.0f;
            float vy = 1.0f + (rand() % 300) / 150.0f;
            Particle p;
            p.pos = worldPos + glm::vec3(0, 0.5f, 0);
            p.vel = glm::vec3(cosf(a)*speed, vy, sinf(a)*speed);
            p.life = 1.2f + (rand()%100)/200.0f;
            particles.push_back(p);
        }
    }

    void draw(Shader& shader, float dt) {
        if (!alive && !exploding) return;

        float bob = sinf(bobTime * 3.0f) * 0.08f;
        glm::vec3 base = worldPos + glm::vec3(0.0f, 0.5f + bob, 0.0f);
        float targetAngle = atan2f((float)dx, (float)dz);
        angle = targetAngle; // instant turn (can lerp if desired)

        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0,1,0));

        shader.setVec3("objectColor", color);
        shader.setFloat("emissive", 0.3f);
        shader.setFloat("alpha", 1.0f);

        if (alive) {
            // --- HEAD (sphere) ---
            {
                glm::mat4 m = glm::translate(glm::mat4(1.0f), base + glm::vec3(0,0.55f,0));
                m = glm::scale(m, glm::vec3(0.28f));
                shader.setMat4("model", m);
                shader.setVec3("objectColor", glm::vec3(0.95f, 0.85f, 0.75f));
                mSphere->draw();
            }
            // --- HAT (cone) ---
            {
                glm::mat4 m = glm::translate(glm::mat4(1.0f), base + glm::vec3(0, 0.85f, 0));
                m = glm::scale(m, glm::vec3(0.22f, 0.45f, 0.22f));
                shader.setMat4("model", m);
                shader.setVec3("objectColor", color);
                shader.setFloat("emissive", 0.5f);
                mCone->draw();
            }
            // --- ROBE (cylinder, wide at bottom) ---
            {
                glm::mat4 m = glm::translate(glm::mat4(1.0f), base + glm::vec3(0, 0.1f, 0));
                m = m * rot;
                m = glm::scale(m, glm::vec3(0.38f, 0.55f, 0.38f));
                shader.setMat4("model", m);
                shader.setVec3("objectColor", color * 0.7f);
                shader.setFloat("emissive", 0.1f);
                mCylinder->draw();
            }
            // --- BROOMSTICK (cylinder rotated, sticking forward) ---
            {
                glm::mat4 m = glm::translate(glm::mat4(1.0f), base + glm::vec3(0, -0.08f, 0));
                m = m * rot;
                m = glm::rotate(m, glm::radians(90.0f), glm::vec3(0,0,1));
                // tilt slightly forward
                m = glm::rotate(m, glm::radians(-15.0f), glm::vec3(1,0,0));
                m = glm::scale(m, glm::vec3(0.06f, 1.1f, 0.06f));
                shader.setMat4("model", m);
                shader.setVec3("objectColor", glm::vec3(0.6f, 0.4f, 0.1f));
                shader.setFloat("emissive", 0.0f);
                mCylinder->draw();
            }
            // --- BROOM BRISTLES (cone at back of stick) ---
            {
                glm::mat4 m = glm::translate(glm::mat4(1.0f), base + glm::vec3(0, -0.08f, 0));
                m = m * rot;
                // position at the back end of the broom
                glm::vec3 backOffset(-0.55f, 0.0f, 0.0f);
                m = glm::translate(m, backOffset);
                m = glm::rotate(m, glm::radians(90.0f), glm::vec3(0,0,1));
                m = glm::scale(m, glm::vec3(0.18f, 0.28f, 0.18f));
                shader.setMat4("model", m);
                shader.setVec3("objectColor", glm::vec3(0.5f, 0.35f, 0.08f));
                shader.setFloat("emissive", 0.0f);
                mCone->draw();
            }
        }

        // --- EXPLOSION PARTICLES ---
        if (exploding) {
            shader.setFloat("emissive", 0.8f);
            for (auto& p : particles) {
                float t = 1.0f - p.life / 1.2f;
                shader.setVec3("objectColor", trailColor * (1.0f - t * 0.5f));
                shader.setFloat("alpha", p.life / 1.2f);
                glm::mat4 m = glm::translate(glm::mat4(1.0f), p.pos);
                float s = 0.12f * (1.0f - t * 0.6f);
                m = glm::scale(m, glm::vec3(s));
                shader.setMat4("model", m);
                mSphere->draw();
            }
        }
    }

    void drawTrail(Shader& shader, Mesh& cube) {
        shader.setVec3("objectColor", trailColor);
        shader.setFloat("emissive", 0.5f);
        shader.setFloat("alpha", 1.0f);
        for (auto& t : trail) {
            glm::mat4 m = glm::translate(glm::mat4(1.0f),
                glm::vec3((float)t.x, 0.35f, (float)t.z));
            m = glm::scale(m, glm::vec3(0.9f, 0.7f, 0.9f));
            shader.setMat4("model", m);
            cube.draw();
        }
    }
};

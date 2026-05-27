#pragma once
#include "Wizard.h"
#include <vector>
#include <array>
#include <climits>

class Bot {
public:
    // Decide next direction for the bot wizard
    // Level 1: random avoidance
    // Level 2: greedy open space
    // Level 3: try to cut off player
    static void think(Wizard& bot, const Wizard& player, int gridSize, int level) {
        // Candidate directions: forward, left, right (no 180 reverse)
        struct Dir { int dx, dz; };
        Dir dirs[4] = {{1,0},{-1,0},{0,1},{0,-1}};

        // Gather safe directions
        std::vector<Dir> safe;
        for (auto& d : dirs) {
            if (d.dx == -bot.dx && d.dz == -bot.dz) continue; // no reverse
            int nx = bot.gx + d.dx;
            int nz = bot.gz + d.dz;
            if (!isFree(nx, nz, bot, player, gridSize)) continue;
            safe.push_back(d);
        }

        if (safe.empty()) return; // doomed, keep going

        if (level == 1) {
            // Random among safe options
            Dir chosen = safe[rand() % safe.size()];
            bot.setDirection(chosen.dx, chosen.dz);
            return;
        }

        if (level >= 2) {
            // Greedy: pick direction with most open flood-fill space
            int bestSpace = -1;
            Dir bestDir = safe[0];
            for (auto& d : safe) {
                int nx = bot.gx + d.dx;
                int nz = bot.gz + d.dz;
                int space = floodFill(nx, nz, bot, player, gridSize, 80);
                if (space > bestSpace) {
                    bestSpace = space;
                    bestDir = d;
                }
            }

            if (level == 3) {
                // Also consider directions that move toward player to cut off
                // Pick the greedy-best unless a "cutting" move has >= 60% open space
                Dir cutDir = bestDir;
                int cutSpace = bestSpace;
                for (auto& d : safe) {
                    int nx = bot.gx + d.dx;
                    int nz = bot.gz + d.dz;
                    // Does this direction close distance to player?
                    int distBefore = abs(bot.gx - player.gx) + abs(bot.gz - player.gz);
                    int distAfter  = abs(nx - player.gx) + abs(nz - player.gz);
                    if (distAfter < distBefore) {
                        int sp = floodFill(nx, nz, bot, player, gridSize, 80);
                        if (sp >= cutSpace * 6 / 10) { // at least 60% as open
                            cutDir = d;
                            cutSpace = sp;
                        }
                    }
                }
                bot.setDirection(cutDir.dx, cutDir.dz);
                return;
            }

            bot.setDirection(bestDir.dx, bestDir.dz);
        }
    }

private:
    static bool isFree(int x, int z,
                        const Wizard& bot, const Wizard& player,
                        int gridSize)
    {
        if (x < 0 || x >= gridSize || z < 0 || z >= gridSize) return false;
        for (auto& t : bot.trail)
            if (t.x == x && t.z == z) return false;
        for (auto& t : player.trail)
            if (t.x == x && t.z == z) return false;
        if (player.gx == x && player.gz == z) return false;
        return true;
    }

    // Simple BFS flood fill to count reachable cells (capped at maxCells)
    static int floodFill(int sx, int sz,
                          const Wizard& bot, const Wizard& player,
                          int gridSize, int maxCells)
    {
        if (!isFree(sx, sz, bot, player, gridSize)) return 0;
        std::vector<std::vector<bool>> visited(gridSize, std::vector<bool>(gridSize, false));
        std::vector<std::pair<int,int>> queue;
        queue.push_back({sx, sz});
        visited[sx][sz] = true;
        int count = 0;
        int dx[4]={1,-1,0,0}, dz[4]={0,0,1,-1};
        for (int i = 0; i < (int)queue.size() && count < maxCells; i++) {
            auto [cx, cz] = queue[i];
            count++;
            for (int d = 0; d < 4; d++) {
                int nx = cx+dx[d], nz = cz+dz[d];
                if (nx<0||nx>=gridSize||nz<0||nz>=gridSize) continue;
                if (visited[nx][nz]) continue;
                if (!isFree(nx, nz, bot, player, gridSize)) continue;
                visited[nx][nz] = true;
                queue.push_back({nx, nz});
            }
        }
        return count;
    }
};

#pragma once
#include <vector>
#include <algorithm>

#include "raylib.h"
#include "raymath.h"
#include "npc.h"
#include "settlement.h"

// ===== BOMB =====
struct World; // forward declaration

struct Bomb {
    float x, y;
    float timer;
    float radius;
    bool exploded = false;

    Bomb(float x, float y, float fuse_time = 2.0f, float explosion_radius = 60.0f);

    void Update(World& world, float dt);
    void Explode(World& world);
};

// ===== EXPLOSION EFFECT =====
struct ExplosionEffect {
    float x, y;
    float radius = 0.0f;
    float life = 0.0f;
    float maxLife = 0.4f;
    float maxRadius = 80.0f;

    void Update(float dt);
    bool IsAlive() const { return life < maxLife; }
};

// ===== WORLD =====
struct World {
    int worldW = 1400;
    int worldH = 900;
    int cols = 0;
    int rows = 0;

    std::vector<Settlement> settlements;
    std::vector<NPC> npcs;
    std::vector<Bomb> bombs;
    std::vector<ExplosionEffect> explosionEffects; // теперь ExplosionEffect известен

    void Init();
    void Update(float dt);
    void Draw() const;

    // --- bandit spawning ---
    float banditSpawnTimer = 0.0f;
    int nextBanditGroupId = 1;
    void SpawnCivilian(Vector2 pos);
    void SpawnWarrior(Vector2 pos);
    void MergeSettlementsIfNeeded();

    void AddBomb(float x, float y);
};



inline float RandomFloat(float min, float max) {
    return min + (float)GetRandomValue(0, 10000) / 10000.0f * (max - min);
}
#pragma once

#include "raylib.h"
#include <vector>
#include <cmath>
#include <unordered_set>

struct Barracks {
    bool alive = true;
    Vector2 posPx{0, 0};

    float hp = 600.0f;
    float maxHp = 600.0f;

    float warriorTimer = 10.0f;
    float captainTimer = 150.0f;
};

struct Settlement {
    bool alive = true;
    Color color{255, 255, 255, 255};

    // Settlement territory as tile ids
    std::unordered_set<int> tiles;

    // Cached geometry in world pixels
    Vector2 centerPx{0, 0};
    Rectangle boundsPx{0, 0, 0, 0};
    Vector2 campfirePosPx{};

    // Merge progression and barracks ownership
    int sourceSettlementCount = 1;
    std::vector<Barracks> barracksList;

    // Settlement war state
    bool warActive = false;
    int warTargetSettlementId = -1;

    bool attackWaveLaunched = false;
    float warRecruitCooldown = 0.0f;
    int warWaveSize = 0;

    // War staging and squad preparation
    int preparedSquadCount = 0;
    bool offensiveWaveReady = false;
    bool defensiveMobilization = false;
};

static constexpr int CELL_SIZE = 8;

// Converts a tile coordinate to its pixel-center position
inline Vector2 CellToPxCenter(int cx, int cy) {
    return { cx * CELL_SIZE + CELL_SIZE * 0.5f, cy * CELL_SIZE + CELL_SIZE * 0.5f };
}

// Checks whether a point is inside the settlement bounds
inline bool PointInSettlementPx(const Settlement& s, Vector2 pPx) {
    if (!s.alive) return false;

    return CheckCollisionPointRec(pPx, s.boundsPx);
}

// Safely normalizes a 2D vector
inline Vector2 SafeNormalize(Vector2 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y);
    if (len <= 0.0001f) return {0.0f, 0.0f};
    return { v.x / len, v.y / len };
}

// Returns a random unit vector in 2D
inline Vector2 RandomUnit2D() {
    float a = GetRandomValue(0, 360) * DEG2RAD;
    return { cosf(a), sinf(a) };
}

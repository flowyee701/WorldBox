#pragma once

#include "raylib.h"
#include <vector>
#include <cmath>
#include <unordered_set>


struct Settlement {
    bool alive = true;
    Color color{255,255,255,255};

    // НОВОЕ: территория как набор клеток
    std::unordered_set<int> tiles;

    // геометрия (в пикселях)
    Vector2 centerPx{0,0};
    Rectangle boundsPx{0,0,0,0};
    Vector2 campfirePosPx{};  // позиция костра (в пикселях)

    // merged settlement progression / barracks
    int sourceSettlementCount = 1;   // starts as 1, grows on merges
    bool hasBarracks = false;
    Vector2 barracksPosPx{0,0};
    float barracksWarriorTimer = 10.0f;
    float barracksCaptainTimer = 150.0f;

    // settlement war state
    bool warActive = false;
    int warTargetSettlementId = -1;

    bool attackWaveLaunched = false;
    float warRecruitCooldown = 0.0f;
    int warWaveSize = 0;

    // war staging / squad preparation
    int preparedSquadCount = 0;
    bool offensiveWaveReady = false;
    bool defensiveMobilization = false;

    // legacy — больше НЕ используем
    // std::vector<Rectangle> zones;
};

static constexpr int CELL_SIZE = 8;

inline Vector2 CellToPxCenter(int cx, int cy) {
    return { cx * CELL_SIZE + CELL_SIZE * 0.5f, cy * CELL_SIZE + CELL_SIZE * 0.5f };
}

inline bool PointInSettlementPx(const Settlement& s, Vector2 pPx) {
    if (!s.alive) return false;

    // Быстрая и стабильная проверка: точка в прямоугольнике границ поселения
    // (boundsPx должен быть посчитан при создании/мердже поселения)
    return CheckCollisionPointRec(pPx, s.boundsPx);
}

inline Vector2 SafeNormalize(Vector2 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y);
    if (len <= 0.0001f) return {0.0f, 0.0f};
    return { v.x / len, v.y / len };
}

inline Vector2 RandomUnit2D() {
    float a = GetRandomValue(0, 360) * DEG2RAD;
    return { cosf(a), sinf(a) };
}

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

    // legacy — больше НЕ используем
    // std::vector<Rectangle> zones;
};

static constexpr int CELL_SIZE = 8;

inline Vector2 CellToPxCenter(int cx, int cy) {
    return { cx * CELL_SIZE + CELL_SIZE * 0.5f, cy * CELL_SIZE + CELL_SIZE * 0.5f };
}

inline bool PointInSettlementPx(const Settlement& s, Vector2 pPx) {
    int cx = static_cast<int>(pPx.x / CELL_SIZE);
    int cy = static_cast<int>(pPx.y / CELL_SIZE);

    return false;
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


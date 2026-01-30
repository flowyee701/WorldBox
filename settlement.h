#pragma once
#include "raylib.h"
#include <vector>
#include <cmath>


struct Settlement {
    // клетки (а не пиксели)
    std::vector<Rectangle> zones; // x,y,w,h в КЛЕТКАХ
    Vector2 centerPx{};           // центр в ПИКСЕЛЯХ
    Color color{WHITE};
    bool alive = true;
};

static constexpr int CELL_SIZE = 8;

inline Vector2 CellToPxCenter(int cx, int cy) {
    return { cx * CELL_SIZE + CELL_SIZE * 0.5f, cy * CELL_SIZE + CELL_SIZE * 0.5f };
}

inline bool PointInSettlementPx(const Settlement& s, Vector2 pPx) {
    int cx = static_cast<int>(pPx.x / CELL_SIZE);
    int cy = static_cast<int>(pPx.y / CELL_SIZE);

    for (const auto& r : s.zones) {
        int rx = (int)r.x, ry = (int)r.y, rw = (int)r.width, rh = (int)r.height;
        if (cx >= rx && cx < rx + rw && cy >= ry && cy < ry + rh) return true;
    }
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

inline Vector2 ComputeSettlementCenterPx(const Settlement& s) {
    double sumArea = 0.0, sumX = 0.0, sumY = 0.0;

    for (const auto& r : s.zones) {
        double rx = r.x, ry = r.y, rw = r.width, rh = r.height;
        double area = rw * rh;
        sumArea += area;
        sumX += (rx + rw * 0.5) * area;
        sumY += (ry + rh * 0.5) * area;
    }
    if (sumArea <= 0.0) return {0, 0};

    double cx = sumX / sumArea;
    double cy = sumY / sumArea;
    return CellToPxCenter((int)cx, (int)cy);
}
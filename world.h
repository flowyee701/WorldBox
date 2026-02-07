#pragma once
#include <vector>
#include <algorithm>

#include "raylib.h"
#include "raymath.h"
#include "npc.h"
#include "settlement.h"


struct World {
    int worldW = 1400;
    int worldH = 900;
    int cols = 0;
    int rows = 0;

    std::vector<Settlement> settlements;
    std::vector<NPC> npcs;
    Vector2 ComputeSettlementCenterPx(const Settlement& s);
    Rectangle ComputeSettlementBoundsPx(const Settlement& s);

    void Init();
    void Update(float dt);
    void Draw() const;

    // --- bandit spawning ---
    float banditSpawnTimer = 0.0f;
    int nextBanditGroupId = 1;
    void SpawnCivilian(Vector2 pos);
    void SpawnWarrior(Vector2 pos);
    void MergeSettlementsIfNeeded();
};
inline float RandomFloat(float min, float max) {
    return min + (float)GetRandomValue(0, 10000) / 10000.0f * (max - min);

}

#pragma once
#include <vector>
#include <algorithm>

#include "raylib.h"
#include "raymath.h"
#include "npc/npc.h"
#include "settlement.h"


struct Settlement;

class World {
public:
    int worldW = 0;
    int worldH = 0;
    int cols = 0;
    int rows = 0;

    std::vector<Settlement> settlements;
    std::vector<NPC> npcs;

    static constexpr int NPC_VARIANTS = 3;

    Texture2D npcTexCivilian[NPC_VARIANTS]{};
    Texture2D npcTexWarrior[NPC_VARIANTS]{};
    Texture2D npcTexBandit[NPC_VARIANTS]{};

    bool npcTexCivilianLoaded[NPC_VARIANTS]{};
    bool npcTexWarriorLoaded[NPC_VARIANTS]{};
    bool npcTexBanditLoaded[NPC_VARIANTS]{};

    bool npcSpritesLoaded = false;

    void LoadNpcSprites();
    void UnloadNpcSprites();


    // ✅ ВОТ ЭТО ВАЖНО
    float banditSpawnTimer = 0.0f;
    int nextBanditGroupId = 1;


    void Init();
    void Update(float dt);
    void Draw() const;

    bool PointInSettlementPx(const Settlement& s, Vector2 pos) const;
    Vector2 ComputeSettlementCenterPx(const Settlement& s);
    Rectangle ComputeSettlementBoundsPx(const Settlement& s);

    void MergeSettlementsIfNeeded();
    void SpawnCivilian(Vector2 pos);
    void SpawnWarrior(Vector2 pos);

    void Shutdown();
};
inline float RandomFloat(float min, float max) {
    return min + (float)GetRandomValue(0, 10000) / 10000.0f * (max - min);

};
#pragma once
#include <vector>
#include <algorithm>

#include <raylib.h>
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
    bool npcTexCivilianLoaded[NPC_VARIANTS]{};

    Texture2D npcTexWarrior[NPC_VARIANTS]{};
    bool npcTexWarriorLoaded[NPC_VARIANTS]{};

    Texture2D npcTexBandit[NPC_VARIANTS]{};
    bool npcTexBanditLoaded[NPC_VARIANTS]{};

    Texture2D npcTexCaptain[NPC_VARIANTS]{};
    bool npcTexCaptainLoaded[NPC_VARIANTS]{};

    bool npcSpritesLoaded = false;

    void LoadNpcSprites();
    void UnloadNpcSprites();


    // ✅ ВОТ ЭТО ВАЖНО
    float banditSpawnTimer = 0.0f;
    int nextBanditGroupId = 1;
    void SpawnCaptain(Vector2 pos);
    Texture2D captainTex{};
    bool captainTexLoaded = false;
    // ===== Campfire =====
    static constexpr int FIRE_FRAMES = 4;

    Texture2D fireTex[FIRE_FRAMES]{};
    bool fireLoaded[FIRE_FRAMES]{false};

    int fireFrame = 0;
    float fireAnimT = 0.0f;
    float fireAnimSpeed = 0.10f; // 0.10 = 10 кадров/сек (можно 0.12..0.18)

    // ===== Barracks =====
    Texture2D barracksTex{};
    bool barracksTexLoaded = false;

    // -------------------------------------------------
    // NPC ids & captain selection
    // -------------------------------------------------
    uint32_t nextNpcId = 1;           // ids start at 1
    uint32_t selectedCaptainId = 0;   // 0 = none selected
    int selectedCaptainIndex = -1; // индекс в npcs, -1 если никто не выбран

    NPC* FindNpcById(uint32_t id);
    const NPC* FindNpcById(uint32_t id) const;
    void StartSettlementWar(int attackerSettlementId, int targetSettlementId);
    void StopSettlementWar(int settlementId);
    bool IsSettlementAliveAndValid(int settlementId) const;
    void BeginNpcDeath(NPC& npc);
    void BeginNpcAttack(NPC& npc, Vector2 targetPos);

    void IssueCaptainMoveOrder(uint32_t captainId, Vector2 targetPx);

    void LoadFireSprites();
    void UnloadFireSprites();
    void UpdateCampfires(); // пересчёт позиций костров по центру поселений
    void LoadBarracksSprite();
    void UnloadBarracksSprite();
    void UpdateBarracks();
    void UpdateBarracksProduction(float dt);
    void UpdateSettlementWars(float dt);
    void UpdateSettlementWarAssignments();
    void UpdateSettlementWarPreparation(float dt);
    void UpdateSettlementDefense(float dt);
    void RefreshSettlementWarSquads();


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

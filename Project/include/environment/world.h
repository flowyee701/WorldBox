#pragma once
#include <algorithm>
#include <vector>
#include <memory>
#include <raylib.h>
#include "raymath.h"
#include "npc/npc.h"
#include "settlement.h"
#include "terrain/terrain.h"

#include "npc/Animal.h"
#include "environment/Plant.h"

struct Settlement;

class World {
public:
    int worldW = 0;
    int worldH = 0;
    int cols = 0;
    int rows = 0;

    Terrain terrain;
    unsigned int worldSeed = 0;

    std::vector<Settlement> settlements;
    std::vector<NPC> npcs;

    // --- ДОБАВЛЕНО: Списки для хранения растений и животных ---
    std::vector<std::unique_ptr<Animal>> animals;
    std::vector<Plant> plants;

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

    // NPC sprite resources
    void LoadNpcSprites();
    void UnloadNpcSprites();

    // Bandit spawning state
    float banditSpawnTimer = 0.0f;
    int nextBanditGroupId = 1;

    // Captain resources and spawning
    void SpawnCaptain(Vector2 pos);
    Texture2D captainTex{};
    bool captainTexLoaded = false;

    // Campfire resources
    static constexpr int FIRE_FRAMES = 4;
    Texture2D fireTex[FIRE_FRAMES]{};
    bool fireLoaded[FIRE_FRAMES]{false};
    int fireFrame = 0;
    float fireAnimT = 0.0f;
    float fireAnimSpeed = 0.10f;

    // Barracks resources
    Texture2D barracksTex{};
    bool barracksTexLoaded = false;

    // NPC ids and captain selection
    uint32_t nextNpcId = 1;
    uint32_t selectedCaptainId = 0;
    int selectedCaptainIndex = -1;

    NPC* FindNpcById(uint32_t id);
    const NPC* FindNpcById(uint32_t id) const;
    bool TryBuildBarracksAt(Vector2 worldPos);
    void StartSettlementWar(int attackerSettlementId, int targetSettlementId);
    void StopSettlementWar(int settlementId);
    bool IsSettlementAliveAndValid(int settlementId) const;
    void BeginNpcDeath(NPC& npc);
    void BeginNpcAttack(NPC& npc, Vector2 targetPos);
    bool SettlementHasLivingCombatUnits(int settlementId) const;
    void DamageSettlementBarracks(int settlementId, int barracksIndex, float damage);

    void IssueCaptainMoveOrder(uint32_t captainId, Vector2 targetPx);

    void LoadFireSprites();
    void UnloadFireSprites();
    void UpdateCampfires();
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

    // --- ДОБАВЛЕНО: Методы для спавна природы ---
    void SpawnAnimal(Vector2 pos);
    void SpawnPlant(Vector2 pos, float treeChance = 0.5f);
    void GenerateNature(int plantCount, int animalCount);

    void Shutdown();
};

inline float RandomFloat(float min, float max) {
    return min + (float)GetRandomValue(0, 10000) / 10000.0f * (max - min);
}

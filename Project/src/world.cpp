#include <optional>
#include <stdexcept>
#include "environment/world.h"
#include "npc/human_behavior.h"
#include "npc/bandit_behavior.h"
#include "environment/settlement.h"
#include "npc/npc.h"
#include <algorithm>
#include <cfloat>
#include <iostream>
#include <string>
#include "npc/Animal.h"
#include "environment/Plant.h"
// ------------------------------------------------------------

// Returns a random spawn position on the world edge
static Vector2 RandomOutsideSpawn(int w, int h) {
    int side = GetRandomValue(0, 3);
    switch (side) {
        case 0: return {0.0f,        (float)GetRandomValue(0, h)};
        case 1: return {(float)w,    (float)GetRandomValue(0, h)};
        case 2: return {(float)GetRandomValue(0, w), 0.0f};
        default:return {(float)GetRandomValue(0, w), (float)h};
    }
}
static float ClampF(float v, float a, float b) { return (v < a) ? a : (v > b) ? b : v; }
static Vector2 NearestTileCenterPx(const World& world, const Settlement& s, Vector2 from)
{
    if (s.tiles.empty()) return from;

    float bestD2 = 1e30f;
    Vector2 best = from;

    for (int tile : s.tiles) {
        int cx = tile % world.cols;
        int cy = tile / world.cols;

        Vector2 c{
                (cx + 0.5f) * CELL_SIZE,
                (cy + 0.5f) * CELL_SIZE
        };

        float dx = c.x - from.x;
        float dy = c.y - from.y;
        float d2 = dx*dx + dy*dy;

        if (d2 < bestD2) { bestD2 = d2; best = c; }
    }
    return best;
}
static void ClampNpcInsideWorld(NPC& npc, int worldW, int worldH) {
    float oldX = npc.pos.x;
    float oldY = npc.pos.y;

    npc.pos.x = ClampF(npc.pos.x, 0.0f, (float)worldW);
    npc.pos.y = ClampF(npc.pos.y, 0.0f, (float)worldH);

    if (npc.pos.x != oldX) npc.vel.x = 0.0f;
    if (npc.pos.y != oldY) npc.vel.y = 0.0f;
}
static Vector2 ClosestPointInRectPx(Vector2 p, const Rectangle& rPx)
{
    return {
            ClampF(p.x, rPx.x, rPx.x + rPx.width),
            ClampF(p.y, rPx.y, rPx.y + rPx.height)
    };
}
static std::string PathJoin(const char* a, const char* b) {
    std::string s = a;
    if (!s.empty() && s.back() != '/') s += "/";
    s += b;
    return s;
}

static bool RectsOverlap(const Rectangle &a, const Rectangle &b) {
    return (a.x < b.x + b.width) && (a.x + a.width > b.x) &&
           (a.y < b.y + b.height) && (a.y + a.height > b.y);
}

static Rectangle ZoneToPxRect(const Rectangle &z) {
    return Rectangle{
            z.x * CELL_SIZE,
            z.y * CELL_SIZE,
            z.width * CELL_SIZE,
            z.height * CELL_SIZE
    };
}

static float Dist2World(Vector2 a, Vector2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

static Vector2 TileIdToCenterPx(const World& world, int tileId) {
    int cx = tileId % world.cols;
    int cy = tileId / world.cols;
    return {
            (cx + 0.5f) * CELL_SIZE,
            (cy + 0.5f) * CELL_SIZE
    };
}

static Vector2 PickRandomSettlementTileFarFrom(const World& world,
                                               const Settlement& s,
                                               Vector2 avoidPx,
                                               float minDistPx)
{
    if (s.tiles.empty()) return s.centerPx;

    std::vector<int> preferred;
    std::vector<int> fallback;
    preferred.reserve(s.tiles.size());
    fallback.reserve(s.tiles.size());

    float minD2 = minDistPx * minDistPx;

    for (int tile : s.tiles) {
        Vector2 p = TileIdToCenterPx(world, tile);
        fallback.push_back(tile);

        if (Dist2World(p, avoidPx) >= minD2) {
            preferred.push_back(tile);
        }
    }

    const std::vector<int>& pool = !preferred.empty() ? preferred : fallback;
    int pickIndex = GetRandomValue(0, (int)pool.size() - 1);
    return TileIdToCenterPx(world, pool[pickIndex]);
}

static bool IsCombatHumanRole(NPC::HumanRole role) {
    return role == NPC::HumanRole::WARRIOR || role == NPC::HumanRole::CAPTAIN;
}

static bool IsCombatRoleForWar(NPC::HumanRole role) {
    return role == NPC::HumanRole::WARRIOR || role == NPC::HumanRole::CAPTAIN;
}

static bool IsCombatHumanRoleForBattle(NPC::HumanRole role) {
    return role == NPC::HumanRole::WARRIOR || role == NPC::HumanRole::CAPTAIN;
}

static int CountEnemyCombatUnitsNearNpc(const World& world, const NPC& npc, float radiusPx) {
    float radius2 = radiusPx * radiusPx;
    int count = 0;

    for (const auto& other : world.npcs) {
        if (!other.alive || other.isDying) continue;
        if (!IsCombatHumanRoleForBattle(other.humanRole)) continue;
        if (other.settlementId == npc.settlementId) continue;

        float dx = other.pos.x - npc.pos.x;
        float dy = other.pos.y - npc.pos.y;
        float d2 = dx*dx + dy*dy;
        if (d2 <= radius2) {
            count++;
        }
    }

    return count;
}

static int FindNearestEnemyCombatNearNpc(World& world, const NPC& npc, float radiusPx) {
    float bestD2 = radiusPx * radiusPx;
    int bestIndex = -1;
    int bestPriority = 999;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const NPC& other = world.npcs[i];
        if (!other.alive || other.isDying) continue;
        if (other.settlementId == npc.settlementId) continue;

        int priority = 999;
        if (other.humanRole == NPC::HumanRole::WARRIOR) priority = 0;
        else if (other.humanRole == NPC::HumanRole::CAPTAIN) priority = 1;
        else continue;

        float dx = other.pos.x - npc.pos.x;
        float dy = other.pos.y - npc.pos.y;
        float d2 = dx*dx + dy*dy;
        if (d2 > bestD2) continue;

        if (priority < bestPriority || (priority == bestPriority && d2 < bestD2)) {
            bestPriority = priority;
            bestD2 = d2;
            bestIndex = i;
        }
    }

    return bestIndex;
}

static bool IsEnemyCombatNearNpc(const World& world, const NPC& npc, float radiusPx) {
    float radius2 = radiusPx * radiusPx;

    for (const auto& other : world.npcs) {
        if (!other.alive || other.isDying) continue;
        if (!IsCombatHumanRoleForBattle(other.humanRole)) continue;
        if (other.settlementId == npc.settlementId) continue;

        float dx = other.pos.x - npc.pos.x;
        float dy = other.pos.y - npc.pos.y;
        float d2 = dx*dx + dy*dy;
        if (d2 <= radius2) return true;
    }

    return false;
}

static bool IsCombatNpc(const NPC& npc) {
    return npc.humanRole == NPC::HumanRole::WARRIOR || npc.humanRole == NPC::HumanRole::CAPTAIN;
}

static bool IsCaptainNpc(const NPC& npc) {
    return npc.humanRole == NPC::HumanRole::CAPTAIN;
}

static int CountAvailableSettlementCombatUnits(const World& world, int settlementId) {
    int count = 0;
    for (const auto& npc : world.npcs) {
        if (!npc.alive) continue;
        if (npc.isDying) continue;
        if (npc.settlementId != settlementId) continue;
        if (!IsCombatHumanRole(npc.humanRole)) continue;
        count++;
    }
    return count;
}

static int CountAssignedWarriorsForCaptain(const World& world, int settlementId, uint32_t captainId) {
    int count = 0;
    for (const auto& npc : world.npcs) {
        if (!npc.alive || npc.isDying) continue;
        if (npc.settlementId != settlementId) continue;
        if (npc.humanRole != NPC::HumanRole::WARRIOR) continue;
        if (npc.warCaptainId == captainId) count++;
    }
    return count;
}

static int CountReadySquadsForSettlement(const World& world, int settlementId) {
    int readySquads = 0;

    for (const auto& npc : world.npcs) {
        if (!npc.alive || npc.isDying) continue;
        if (npc.settlementId != settlementId) continue;
        if (npc.humanRole != NPC::HumanRole::CAPTAIN) continue;

        int warriorCount = CountAssignedWarriorsForCaptain(world, settlementId, npc.id);

        // valid offensive/defensive squad:
        // 1 captain + at least 2 warriors
        if (warriorCount >= 2) {
            readySquads++;
        }
    }

    return readySquads;
}

static uint32_t FindNearestAvailableCaptainId(World& world, int settlementId, Vector2 pos) {
    float bestD2 = 1e30f;
    uint32_t bestId = 0;

    for (auto& npc : world.npcs) {
        if (!npc.alive || npc.isDying) continue;
        if (npc.settlementId != settlementId) continue;
        if (npc.humanRole != NPC::HumanRole::CAPTAIN) continue;

        int warriorCount = CountAssignedWarriorsForCaptain(world, settlementId, npc.id);
        if (warriorCount >= 5) continue;

        float dx = npc.pos.x - pos.x;
        float dy = npc.pos.y - pos.y;
        float d2 = dx*dx + dy*dy;
        if (d2 < bestD2) {
            bestD2 = d2;
            bestId = npc.id;
        }
    }

    return bestId;
}

static bool IsEnemySettlementTroopNearSettlement(const World& world, int settlementId, float radiusPx) {
    if (settlementId < 0 || settlementId >= (int)world.settlements.size()) return false;
    const Settlement& s = world.settlements[settlementId];
    if (!s.alive) return false;

    float radius2 = radiusPx * radiusPx;

    for (const auto& npc : world.npcs) {
        if (!npc.alive || npc.isDying) continue;
        if (!IsCombatNpc(npc)) continue;
        if (npc.settlementId == settlementId) continue;
        if (!npc.warAssigned) continue;

        float dx = npc.pos.x - s.centerPx.x;
        float dy = npc.pos.y - s.centerPx.y;
        float d2 = dx*dx + dy*dy;
        if (d2 <= radius2) return true;
    }

    return false;
}

static int ComputeSettlementWaveSize(const World& world, int settlementId) {
    int available = CountAvailableSettlementCombatUnits(world, settlementId);
    int preferred = (int)floorf((float)available * 0.60f);
    if (preferred < 4) preferred = 4;
    if (preferred > 12) preferred = 12;
    return preferred;
}

static int FindNearestEnemyNpcForSettlementWar(World& world, const NPC& attacker, int enemySettlementId, float maxDistPx) {
    float bestD2 = maxDistPx * maxDistPx;
    int bestIndex = -1;
    int bestPriority = 999;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const NPC& other = world.npcs[i];
        if (!other.alive || other.isDying) continue;
        if (other.id == attacker.id) continue;

        if (other.settlementId != enemySettlementId) continue;

        int priority = 999;
        if (other.humanRole == NPC::HumanRole::WARRIOR) priority = 0;
        else if (other.humanRole == NPC::HumanRole::CAPTAIN) priority = 1;
        else if (other.humanRole == NPC::HumanRole::CIVILIAN) priority = 2;
        else continue;

        float dx = other.pos.x - attacker.pos.x;
        float dy = other.pos.y - attacker.pos.y;
        float d2 = dx*dx + dy*dy;

        if (d2 > bestD2) continue;

        if (priority < bestPriority || (priority == bestPriority && d2 < bestD2)) {
            bestPriority = priority;
            bestD2 = d2;
            bestIndex = i;
        }
    }

    return bestIndex;
}

static bool IsBarracksInAttackRange(const Settlement& s, Vector2 pos, float rangePx)
{
    if (!s.alive) return false;

    float range2 = rangePx * rangePx;
    for (const auto& b : s.barracksList) {
        if (!b.alive || b.hp <= 0.0f) continue;

        float dx = b.posPx.x - pos.x;
        float dy = b.posPx.y - pos.y;
        if ((dx*dx + dy*dy) <= range2) return true;
    }

    return false;
}

static int FindNearestAliveBarracksIndex(const Settlement& s, Vector2 fromPos)
{
    float bestD2 = 1e30f;
    int bestIndex = -1;

    for (int i = 0; i < (int)s.barracksList.size(); i++) {
        const Barracks& b = s.barracksList[i];
        if (!b.alive) continue;
        if (b.hp <= 0.0f) continue;

        float dx = b.posPx.x - fromPos.x;
        float dy = b.posPx.y - fromPos.y;
        float d2 = dx * dx + dy * dy;

        if (d2 < bestD2) {
            bestD2 = d2;
            bestIndex = i;
        }
    }

    return bestIndex;
}

static int FindNearestHostileTroopNearSettlement(World& world, int homeSettlementId, Vector2 homePos, float maxDistPx) {
    float bestD2 = maxDistPx * maxDistPx;
    int bestIndex = -1;
    int bestPriority = 999;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const NPC& other = world.npcs[i];
        if (!other.alive || other.isDying) continue;
        if (other.settlementId == homeSettlementId) continue;

        int priority = 999;
        if (other.humanRole == NPC::HumanRole::WARRIOR) priority = 0;
        else if (other.humanRole == NPC::HumanRole::CAPTAIN) priority = 1;
        else if (other.humanRole == NPC::HumanRole::CIVILIAN) priority = 2;
        else continue;

        float dx = other.pos.x - homePos.x;
        float dy = other.pos.y - homePos.y;
        float d2 = dx*dx + dy*dy;
        if (d2 > bestD2) continue;

        if (priority < bestPriority || (priority == bestPriority && d2 < bestD2)) {
            bestPriority = priority;
            bestD2 = d2;
            bestIndex = i;
        }
    }

    return bestIndex;
}

static void SpawnProducedWarrior(World& world, int settlementId, Vector2 pos) {
    NPC npc;
    npc.id = world.nextNpcId++;
    npc.type = NPC::Type::HUMAN;
    npc.humanRole = NPC::HumanRole::WARRIOR;
    npc.skinId = (uint16_t)GetRandomValue(0, World::NPC_VARIANTS - 1);
    npc.pos = pos;
    npc.vel = {0,0};
    npc.speed = 35.0f;
    npc.hp = 180.0f;
    npc.damage = 16.0f;
    npc.settlementId = settlementId;
    npc.alive = true;
    world.npcs.push_back(npc);
}

static void SpawnProducedCaptain(World& world, int settlementId, Vector2 pos) {
    NPC npc;
    npc.id = world.nextNpcId++;
    npc.type = NPC::Type::HUMAN;
    npc.humanRole = NPC::HumanRole::CAPTAIN;
    npc.warriorRank = NPC::WarriorRank::CAPTAIN;
    npc.isCaptain = true;
    npc.skinId = (uint16_t)GetRandomValue(0, World::NPC_VARIANTS - 1);
    npc.pos = pos;
    npc.vel = {0, 0};
    npc.speed = 35.0f;
    npc.hp = 260.0f;
    npc.damage = 22.0f;
    npc.captainAutoMode = true;
    npc.captainHasMoveOrder = false;
    npc.captainHasAttackOrder = false;
    npc.captainAttackGroupId = -1;
    npc.captainAttackTargetId = 0;
    npc.settlementId = settlementId;
    npc.alive = true;
    world.npcs.push_back(npc);
}

// Resolves an asset path relative to the working directory
static std::string FindAssetPath(const char* relativePath)
{
    const char* wd = GetWorkingDirectory();

    const char* candidates[] = {
            "",
            "../",
            "../../",
            "../../../"
    };

    for (const char* base : candidates) {
        std::string full = PathJoin(wd, (std::string(base) + relativePath).c_str());
        if (FileExists(full.c_str())) return full;
    }

    return "";
}

// Loads NPC sprite textures
void World::LoadNpcSprites()
{
    if (npcSpritesLoaded) return;

    auto loadOne = [&](Texture2D& outTex, bool& outLoaded, const char* relPath) {
        std::string p = FindAssetPath(relPath);
        if (p.empty()) {
            TraceLog(LOG_ERROR, "NPC SPRITE missing: %s (WD=%s)", relPath, GetWorkingDirectory());
            outLoaded = false;
            return;
        }

        outTex = LoadTexture(p.c_str());
        if (outTex.id == 0) {
            TraceLog(LOG_ERROR, "Failed to LoadTexture: %s", p.c_str());
            outLoaded = false;
            return;
        }

        SetTextureFilter(outTex, TEXTURE_FILTER_POINT);
        SetTextureWrap(outTex, TEXTURE_WRAP_CLAMP);
        outLoaded = true;

        TraceLog(LOG_INFO, "Loaded sprite: %s (%dx%d)", p.c_str(), outTex.width, outTex.height);
    };

    for (int i = 0; i < NPC_VARIANTS; i++) {
        loadOne(npcTexCivilian[i], npcTexCivilianLoaded[i],
                (std::string("assets/npc/civilian/civilian_") + std::to_string(i) + ".png").c_str());

        loadOne(npcTexWarrior[i], npcTexWarriorLoaded[i],
                (std::string("assets/npc/warrior/warrior_") + std::to_string(i) + ".png").c_str());

        loadOne(npcTexBandit[i], npcTexBanditLoaded[i],
                (std::string("assets/npc/bandit/bandit_") + std::to_string(i) + ".png").c_str());

        loadOne(npcTexCaptain[i], npcTexCaptainLoaded[i],
                (std::string("assets/npc/captain/captain_") + std::to_string(i) + ".png").c_str());
    }

    npcSpritesLoaded = true;

    // --- ЗАГРУЗКА ЛОШАДИ ---
    auto horsePath = FindAssetPath("assets/npc/animal/animal.png");

// Используем std::optional: путь либо есть, либо его нет (nullopt)
    std::optional<std::string> horsePathOpt = horsePath.empty() ? std::nullopt : std::make_optional(horsePath);

    if (!horsePathOpt.has_value()) {
        // ТРЕБОВАНИЕ ТЗ: Использование исключений (throw)
        throw std::runtime_error("Critical error: Horse texture path not found!");
    }

    Animal::texture = LoadTexture(horsePathOpt->c_str());
    if (Animal::texture.id > 0) {
        SetTextureFilter(Animal::texture, TEXTURE_FILTER_POINT);
        Animal::textureLoaded = true;
    } else {
        throw std::runtime_error("Failed to load Horse texture into GPU!");
    }

// --- ЗАГРУЗКА РАСТЕНИЙ ---

// Цветок
    auto flowerPath = FindAssetPath("assets/environment/flower/flower.png");
    if (flowerPath.empty()) throw std::runtime_error("Flower texture not found!");
    Plant::texFlower = LoadTexture(flowerPath.c_str());
    SetTextureFilter(Plant::texFlower, TEXTURE_FILTER_POINT);

// Дерево
    auto treePath = FindAssetPath("assets/environment/tree/tree.png");
    if (treePath.empty()) throw std::runtime_error("Tree texture not found!");
    Plant::texTree = LoadTexture(treePath.c_str());
    SetTextureFilter(Plant::texTree, TEXTURE_FILTER_POINT);

    Plant::texturesLoaded = true;
    npcSpritesLoaded = true;
}

void World::UnloadNpcSprites()
{
    for (int i = 0; i < NPC_VARIANTS; i++) {
        if (npcTexCivilianLoaded[i]) { UnloadTexture(npcTexCivilian[i]); npcTexCivilianLoaded[i] = false; }
        if (npcTexWarriorLoaded[i])  { UnloadTexture(npcTexWarrior[i]);  npcTexWarriorLoaded[i]  = false; }
        if (npcTexBanditLoaded[i])   { UnloadTexture(npcTexBandit[i]);   npcTexBanditLoaded[i]   = false; }
        if (npcTexCaptainLoaded[i])  { UnloadTexture(npcTexCaptain[i]);  npcTexCaptainLoaded[i]  = false; }
    }
    npcSpritesLoaded = false;

    if (Animal::textureLoaded) {
        UnloadTexture(Animal::texture);
        Animal::textureLoaded = false;
    }
}

void World::LoadFireSprites()
{
    auto loadOne = [&](Texture2D& outTex, bool& outLoaded, const char* relPath) {
        std::string p = FindAssetPath(relPath);
        if (p.empty()) {
            TraceLog(LOG_ERROR, "FIRE missing: %s (WD=%s)", relPath, GetWorkingDirectory());
            outLoaded = false;
            return;
        }
        outTex = LoadTexture(p.c_str());
        if (outTex.id == 0) {
            TraceLog(LOG_ERROR, "Failed to LoadTexture fire: %s", p.c_str());
            outLoaded = false;
            return;
        }
        SetTextureFilter(outTex, TEXTURE_FILTER_POINT);
        SetTextureWrap(outTex, TEXTURE_WRAP_CLAMP);
        outLoaded = true;
        TraceLog(LOG_INFO, "Loaded fire: %s", p.c_str());
    };

    for (int i = 0; i < FIRE_FRAMES; i++) {
        std::string rel = "assets/environment/fire/fire_" + std::to_string(i) + ".png";
        loadOne(fireTex[i], fireLoaded[i], rel.c_str());
    }
}

void World::UnloadFireSprites()
{
    for (int i = 0; i < FIRE_FRAMES; i++) {
        if (fireLoaded[i]) {
            UnloadTexture(fireTex[i]);
            fireLoaded[i] = false;
        }
    }
}

void World::LoadBarracksSprite()
{
    std::string p = FindAssetPath("assets/barracks/barracks_0.png");
    if (p.empty()) {
        TraceLog(LOG_WARNING, "BARRACKS missing: assets/barracks/barracks_0.png (WD=%s)", GetWorkingDirectory());
        barracksTexLoaded = false;
        return;
    }

    barracksTex = LoadTexture(p.c_str());
    if (barracksTex.id == 0) {
        TraceLog(LOG_ERROR, "Failed to LoadTexture barracks: %s", p.c_str());
        barracksTexLoaded = false;
        return;
    }

    SetTextureFilter(barracksTex, TEXTURE_FILTER_POINT);
    barracksTexLoaded = true;

    TraceLog(LOG_INFO, "Loaded barracks: %s", p.c_str());
}

void World::UnloadBarracksSprite()
{
    if (barracksTexLoaded) {
        UnloadTexture(barracksTex);
        barracksTexLoaded = false;
    }
}

void World::UpdateCampfires()
{
    for (auto& s : settlements) {
        if (!s.alive) continue;

        Vector2 c = s.centerPx;

        // Snap to the nearest settlement tile if the cached center drifts out
        if (!PointInSettlementPx(s, c)) {
            c = NearestTileCenterPx(*this, s, c);
        }

        s.campfirePosPx = c;
    }
}

void World::UpdateBarracks()
{
    for (auto& s : settlements) {
        if (!s.alive) continue;

        int desiredAutoBarracks = s.sourceSettlementCount / 3;
        if (desiredAutoBarracks <= 0) continue;

        int aliveBarracksCount = 0;
        for (const auto& b : s.barracksList) {
            if (b.alive) {
                aliveBarracksCount++;
            }
        }

        while (aliveBarracksCount < desiredAutoBarracks) {
            Vector2 avoid = s.campfirePosPx;
            if (avoid.x == 0.0f && avoid.y == 0.0f) {
                avoid = s.centerPx;
            }

            Vector2 candidatePos = PickRandomSettlementTileFarFrom(*this, s, avoid, CELL_SIZE * 7.0f);

            bool overlaps = false;
            for (const auto& existing : s.barracksList) {
                if (!existing.alive) continue;

                float dx = candidatePos.x - existing.posPx.x;
                float dy = candidatePos.y - existing.posPx.y;
                float d2 = dx * dx + dy * dy;
                if (d2 < (CELL_SIZE * 4.0f) * (CELL_SIZE * 4.0f)) {
                    overlaps = true;
                    break;
                }
            }

            if (overlaps) {
                bool found = false;
                std::vector<int> settlementTiles;
                settlementTiles.reserve(s.tiles.size());
                for (int tile : s.tiles) {
                    settlementTiles.push_back(tile);
                }

                for (int attempt = 0; attempt < 24 && !found; attempt++) {
                    if (settlementTiles.empty()) break;

                    int pickIndex = GetRandomValue(0, (int)settlementTiles.size() - 1);
                    int tileId = settlementTiles[pickIndex];
                    Vector2 p = TileIdToCenterPx(*this, tileId);

                    float dxFire = p.x - s.campfirePosPx.x;
                    float dyFire = p.y - s.campfirePosPx.y;
                    float fireD2 = dxFire * dxFire + dyFire * dyFire;
                    if (fireD2 < (CELL_SIZE * 5.0f) * (CELL_SIZE * 5.0f)) {
                        continue;
                    }

                    bool localOverlap = false;
                    for (const auto& existing : s.barracksList) {
                        if (!existing.alive) continue;

                        float dx = p.x - existing.posPx.x;
                        float dy = p.y - existing.posPx.y;
                        float d2 = dx * dx + dy * dy;
                        if (d2 < (CELL_SIZE * 4.0f) * (CELL_SIZE * 4.0f)) {
                            localOverlap = true;
                            break;
                        }
                    }

                    if (!localOverlap) {
                        candidatePos = p;
                        found = true;
                    }
                }

                if (!found) {
                    break;
                }
            }

            Barracks b;
            b.alive = true;
            b.posPx = candidatePos;
            b.maxHp = 600.0f;
            b.hp = b.maxHp;
            b.warriorTimer = 10.0f;
            b.captainTimer = 150.0f;

            s.barracksList.push_back(b);
            aliveBarracksCount++;
        }
    }
}

void World::UpdateBarracksProduction(float dt)
{
    for (int i = 0; i < (int)settlements.size(); i++) {
        Settlement& s = settlements[i];
        if (!s.alive) continue;

        for (auto& b : s.barracksList) {
            if (!b.alive) continue;
            if (b.hp <= 0.0f) continue;

            b.warriorTimer -= dt;
            b.captainTimer -= dt;

            while (b.warriorTimer <= 0.0f) {
                Vector2 spawnPos = { b.posPx.x, b.posPx.y + CELL_SIZE * 1.2f };
                SpawnProducedWarrior(*this, i, spawnPos);
                b.warriorTimer += 10.0f;
            }

            while (b.captainTimer <= 0.0f) {
                Vector2 spawnPos = { b.posPx.x, b.posPx.y + CELL_SIZE * 1.2f };
                SpawnProducedCaptain(*this, i, spawnPos);
                b.captainTimer += 150.0f;
            }
        }
    }
}

bool World::PointInSettlementPx(const Settlement& s, Vector2 pos) const {
    int cx = (int)(pos.x / CELL_SIZE);
    int cy = (int)(pos.y / CELL_SIZE);

    if (cx < 0 || cy < 0 || cx >= cols || cy >= rows)
        return false;

    int tileId = cy * cols + cx;
    return s.tiles.find(tileId) != s.tiles.end();
}

bool World::SettlementHasLivingCombatUnits(int settlementId) const
{
    if (settlementId < 0 || settlementId >= (int)settlements.size()) return false;
    if (!settlements[settlementId].alive) return false;

    for (const auto& npc : npcs) {
        if (!npc.alive || npc.isDying) continue;
        if (npc.settlementId != settlementId) continue;
        if (!IsCombatRoleForWar(npc.humanRole)) continue;
        return true;
    }
    return false;
}

void World::DamageSettlementBarracks(int settlementId, int barracksIndex, float damage)
{
    if (settlementId < 0 || settlementId >= (int)settlements.size()) return;

    Settlement& s = settlements[settlementId];
    if (!s.alive) return;
    if (barracksIndex < 0 || barracksIndex >= (int)s.barracksList.size()) return;

    Barracks& b = s.barracksList[barracksIndex];
    if (!b.alive) return;
    if (b.hp <= 0.0f) return;

    b.hp -= damage;
    if (b.hp <= 0.0f) {
        b.hp = 0.0f;
        b.maxHp = 0.0f;
        b.alive = false;
    }
}

Vector2 World::ComputeSettlementCenterPx(const Settlement& s) {
    if (s.tiles.empty()) return {0.0f, 0.0f};

    double sx = 0.0, sy = 0.0;
    int n = 0;

    for (int tile : s.tiles) {
        int cx = tile % cols;
        int cy = tile / cols;

        sx += (cx + 0.5) * CELL_SIZE;
        sy += (cy + 0.5) * CELL_SIZE;
        n++;
    }

    return { (float)(sx / n), (float)(sy / n) };
}

Rectangle World::ComputeSettlementBoundsPx(const Settlement& s) {
    if (s.tiles.empty()) return {0,0,0,0};

    int minX = cols, minY = rows;
    int maxX = -1, maxY = -1;

    for (int tile : s.tiles) {
        int cx = tile % cols;
        int cy = tile / cols;

        if (cx < minX) minX = cx;
        if (cy < minY) minY = cy;
        if (cx > maxX) maxX = cx;
        if (cy > maxY) maxY = cy;
    }

    return {
            (float)(minX * CELL_SIZE),
            (float)(minY * CELL_SIZE),
            (float)((maxX - minX + 1) * CELL_SIZE),
            (float)((maxY - minY + 1) * CELL_SIZE)
    };
}


void World::MergeSettlementsIfNeeded() {
    for (int i = 0; i < (int)settlements.size(); i++) {
        if (!settlements[i].alive) continue;

        for (int j = i + 1; j < (int)settlements.size(); j++) {
            if (!settlements[j].alive) continue;

            bool touching = false;

            for (int tile : settlements[j].tiles) {
                if (settlements[i].tiles.count(tile)) {
                    touching = true;
                    break;
                }
            }

            if (!touching) continue;

            if (settlements[j].warActive) {
                StopSettlementWar(j);
            }

            // Merge settlement j into settlement i
            for (int tile : settlements[j].tiles)
                settlements[i].tiles.insert(tile);

            for (auto& npc : npcs)
                if (npc.settlementId == j)
                    npc.settlementId = i;

            settlements[i].sourceSettlementCount += settlements[j].sourceSettlementCount;

            settlements[i].barracksList.insert(settlements[i].barracksList.end(),
                                               settlements[j].barracksList.begin(),
                                               settlements[j].barracksList.end());

            settlements[j].tiles.clear();
            settlements[j].barracksList.clear();
            settlements[j].alive = false;

            settlements[i].centerPx = ComputeSettlementCenterPx(settlements[i]);
            settlements[i].boundsPx = ComputeSettlementBoundsPx(settlements[i]);
        }
    }
}

static Vector2 RandomEdgeSpawn(int w, int h) {
    int side = GetRandomValue(0, 3);
    switch (side) {
        case 0:
            return {0.0f, (float) GetRandomValue(0, h)};        // left
        case 1:
            return {(float) w, (float) GetRandomValue(0, h)};    // right
        case 2:
            return {(float) GetRandomValue(0, w), 0.0f};        // top
        default:
            return {(float) GetRandomValue(0, w), (float) h};   // bottom
    }
}

void World::SpawnCivilian(Vector2 pos) {
    if (!terrain.canBuild(pos.x, pos.y)) return;

    NPC npc;
    npc.id = nextNpcId++;
    npc.type = NPC::Type::HUMAN;
    npc.humanRole = NPC::HumanRole::CIVILIAN;
    npc.skinId = (uint16_t)GetRandomValue(0, 2); // 0..2
    npc.pos = pos;
    npc.vel = {0, 0};
    npc.speed = 15.0f;
    npc.hp = 100.0f;
    npc.alive = true;
    npc.settlementId = -1;
    npc.damage = 0.0f;


    // Join an existing settlement if the click lands inside one
    int sid = -1;
    for (int i = 0; i < (int) settlements.size(); i++) {
        if (!settlements[i].alive) continue;
        if (PointInSettlementPx(settlements[i], pos)) {
            sid = i;
            break;
        }
    }
    if (sid != -1) {
        npc.settlementId = sid;
        npcs.push_back(npc);
        return;
    }

    // Otherwise look for nearby free civilians to form a new settlement
    if (sid == -1) {
        std::vector<int> nearbyFreeCivs;
        for (int i = 0; i < (int) npcs.size(); i++) {
            const auto &o = npcs[i];
            if (!o.alive) continue;
            if (o.humanRole != NPC::HumanRole::CIVILIAN) continue;
            if (o.settlementId != -1) continue;

            float dx = o.pos.x - pos.x;
            float dy = o.pos.y - pos.y;
            if (dx * dx + dy * dy < 90.0f * 90.0f) nearbyFreeCivs.push_back(i);
        }

        // Create a new settlement when two nearby free civilians are found
        if (nearbyFreeCivs.size() >= 2) {
            Settlement s;
            s.alive = true;
            s.sourceSettlementCount = 1;

            int cx = (int)(pos.x / CELL_SIZE);
            int cy = (int)(pos.y / CELL_SIZE);

            constexpr int R = 8;

            for (int dy = -R; dy <= R; dy++) {
                for (int dx = -R; dx <= R; dx++) {
                    int tx = cx + dx;
                    int ty = cy + dy;
                    if (tx < 0 || ty < 0 || tx >= cols || ty >= rows) continue;
                    s.tiles.insert(ty * cols + tx);
                }
            }

            s.color = Color{
                    (unsigned char)GetRandomValue(80,255),
                    (unsigned char)GetRandomValue(80,255),
                    (unsigned char)GetRandomValue(80,255),
                    255
            };

            settlements.push_back(s);
            int sid = (int)settlements.size() - 1;

            settlements[sid].centerPx = ComputeSettlementCenterPx(settlements[sid]);
            settlements[sid].boundsPx = ComputeSettlementBoundsPx(settlements[sid]);

            npc.settlementId = sid;
            npcs[nearbyFreeCivs[0]].settlementId = sid;
            npcs[nearbyFreeCivs[1]].settlementId = sid;
        }
        npcs.push_back(npc);
    }
}

void World::SpawnWarrior(Vector2 pos) {
    if (!terrain.canBuild(pos.x, pos.y)) return;

    NPC npc;
    npc.id = nextNpcId++;
    npc.type = NPC::Type::HUMAN;
    npc.humanRole = NPC::HumanRole::WARRIOR;
    npc.skinId = (uint16_t)GetRandomValue(0, 2);
    npc.pos = pos;
    npc.vel = {0,0};
    npc.speed = 35.0f;
    npc.hp = 180.0f;
    npc.damage =16.0f;
    npc.settlementId = -1;

    // Attach spawned warriors to the clicked settlement when possible
    for (int i = 0; i < (int)settlements.size(); i++) {
        if (!settlements[i].alive) continue;
        if (PointInSettlementPx(settlements[i], pos)) {
            npc.settlementId = i;
            break;
        }
    }

    npcs.push_back(npc);
}
void World::SpawnCaptain(Vector2 pos) {
    if (!terrain.canBuild(pos.x, pos.y)) return;

    NPC npc;
    npc.id = nextNpcId++;
    npc.type = NPC::Type::HUMAN;
    npc.humanRole = NPC::HumanRole::CAPTAIN;
    npc.warriorRank = NPC::WarriorRank::CAPTAIN;
    npc.isCaptain = true;

    npc.skinId = (uint16_t)GetRandomValue(0, NPC_VARIANTS - 1);
    npc.pos = pos;
    npc.vel = {0, 0};

    npc.speed = 35.0f;
    npc.hp = 260.0f;
    npc.damage = 22.0f;

    npc.captainAutoMode = true;
    npc.captainHasMoveOrder = false;
    npc.captainHasAttackOrder = false;
    npc.captainAttackGroupId = -1;
    npc.captainAttackTargetId = 0;

    npc.settlementId = -1;

    for (int i = 0; i < (int)settlements.size(); i++) {
        if (!settlements[i].alive) continue;
        if (PointInSettlementPx(settlements[i], pos)) {
            npc.settlementId = i;
            break;
        }
    }

    npcs.push_back(npc);
}
NPC* World::FindNpcById(uint32_t id) {
    if (id == 0) return nullptr;
    for (auto& n : npcs) {
        if (n.alive && !n.isDying && n.id == id) return &n;
    }
    return nullptr;
}

const NPC* World::FindNpcById(uint32_t id) const {
    if (id == 0) return nullptr;
    for (const auto& n : npcs) {
        if (n.alive && !n.isDying && n.id == id) return &n;
    }
    return nullptr;
}

bool World::TryBuildBarracksAt(Vector2 worldPos)
{
    if (!terrain.canBuild(worldPos.x, worldPos.y)) return false;

    for (auto& s : settlements) {
        if (!s.alive) continue;
        if (!PointInSettlementPx(s, worldPos)) continue;

        float dxFire = worldPos.x - s.campfirePosPx.x;
        float dyFire = worldPos.y - s.campfirePosPx.y;
        float fireDist2 = dxFire * dxFire + dyFire * dyFire;
        if (fireDist2 < (CELL_SIZE * 5.0f) * (CELL_SIZE * 5.0f)) {
            return false;
        }

        int tileX = (int)(worldPos.x / CELL_SIZE);
        int tileY = (int)(worldPos.y / CELL_SIZE);

        if (tileX < 0 || tileX >= cols || tileY < 0 || tileY >= rows) {
            return false;
        }

        int tileId = tileY * cols + tileX;
        if (s.tiles.find(tileId) == s.tiles.end()) {
            return false;
        }

        Vector2 snappedPos = {
            (tileX + 0.5f) * CELL_SIZE,
            (tileY + 0.5f) * CELL_SIZE
        };

        for (const auto& b : s.barracksList) {
            if (!b.alive) continue;

            float dx = snappedPos.x - b.posPx.x;
            float dy = snappedPos.y - b.posPx.y;
            float d2 = dx * dx + dy * dy;
            if (d2 < (CELL_SIZE * 4.0f) * (CELL_SIZE * 4.0f)) {
                return false;
            }
        }

        Barracks b;
        b.alive = true;
        b.posPx = snappedPos;
        b.maxHp = 600.0f;
        b.hp = b.maxHp;
        b.warriorTimer = 10.0f;
        b.captainTimer = 150.0f;

        s.barracksList.push_back(b);

        return true;
    }

    return false;
}

bool World::IsSettlementAliveAndValid(int settlementId) const
{
    return settlementId >= 0 &&
           settlementId < (int)settlements.size() &&
           settlements[settlementId].alive;
}

void World::StartSettlementWar(int attackerSettlementId, int targetSettlementId)
{
    if (attackerSettlementId == targetSettlementId) return;
    if (!IsSettlementAliveAndValid(attackerSettlementId)) return;
    if (!IsSettlementAliveAndValid(targetSettlementId)) return;

    Settlement& a = settlements[attackerSettlementId];
    Settlement& b = settlements[targetSettlementId];

    a.warActive = true;
    a.warTargetSettlementId = targetSettlementId;
    a.attackWaveLaunched = false;
    a.warRecruitCooldown = 0.0f;
    a.warWaveSize = 0;
    a.preparedSquadCount = 0;
    a.offensiveWaveReady = false;
    a.defensiveMobilization = false;

    b.warActive = true;
    b.warTargetSettlementId = attackerSettlementId;
    b.attackWaveLaunched = false;
    b.warRecruitCooldown = 0.0f;
    b.warWaveSize = 0;
    b.preparedSquadCount = 0;
    b.offensiveWaveReady = false;
    b.defensiveMobilization = false;
}

void World::StopSettlementWar(int settlementId)
{
    if (!IsSettlementAliveAndValid(settlementId)) return;

    Settlement& s = settlements[settlementId];
    int targetId = s.warTargetSettlementId;

    s.warActive = false;
    s.warTargetSettlementId = -1;
    s.attackWaveLaunched = false;
    s.warRecruitCooldown = 0.0f;
    s.warWaveSize = 0;
    s.preparedSquadCount = 0;
    s.offensiveWaveReady = false;
    s.defensiveMobilization = false;

    for (auto& npc : npcs) {
        if (npc.warFromSettlementId == settlementId || npc.warTargetSettlementId == settlementId) {
            npc.warAssigned = false;
            npc.warFromSettlementId = -1;
            npc.warTargetSettlementId = -1;
            npc.warMarching = false;
            npc.warTargetPos = {0.0f, 0.0f};
            npc.warCaptainId = 0;
            npc.warSquadIndex = -1;
            npc.warIsDefender = false;
            npc.warReady = false;
            npc.warInBattle = false;
            npc.warBattleLockTimer = 0.0f;
        }
    }

    if (IsSettlementAliveAndValid(targetId) && settlements[targetId].warTargetSettlementId == settlementId) {
        settlements[targetId].warActive = false;
        settlements[targetId].warTargetSettlementId = -1;
        settlements[targetId].attackWaveLaunched = false;
        settlements[targetId].warRecruitCooldown = 0.0f;
        settlements[targetId].warWaveSize = 0;
        settlements[targetId].preparedSquadCount = 0;
        settlements[targetId].offensiveWaveReady = false;
        settlements[targetId].defensiveMobilization = false;

        for (auto& npc : npcs) {
            if (npc.warFromSettlementId == targetId || npc.warTargetSettlementId == targetId) {
                npc.warAssigned = false;
                npc.warFromSettlementId = -1;
                npc.warTargetSettlementId = -1;
                npc.warMarching = false;
                npc.warTargetPos = {0.0f, 0.0f};
                npc.warCaptainId = 0;
                npc.warSquadIndex = -1;
                npc.warIsDefender = false;
                npc.warReady = false;
                npc.warInBattle = false;
                npc.warBattleLockTimer = 0.0f;
            }
        }
    }
}

void World::RefreshSettlementWarSquads()
{
    // Clear broken captain references
    for (auto& npc : npcs) {
        if (!npc.alive || npc.isDying) continue;
        if (npc.humanRole != NPC::HumanRole::WARRIOR) continue;

        if (npc.warCaptainId != 0) {
            NPC* cap = FindNpcById(npc.warCaptainId);
            if (!cap || cap->settlementId != npc.settlementId || cap->humanRole != NPC::HumanRole::CAPTAIN) {
                npc.warCaptainId = 0;
                npc.warSquadIndex = -1;
                npc.warReady = false;
            }
        }
    }

    // Assign free warriors to nearest available captain inside same settlement
    for (auto& npc : npcs) {
        if (!npc.alive || npc.isDying) continue;
        if (npc.humanRole != NPC::HumanRole::WARRIOR) continue;
        if (npc.settlementId < 0) continue;
        if (npc.warAssigned) continue; // do not rewire active marching/defending units
        if (npc.warCaptainId != 0) continue;

        uint32_t captainId = FindNearestAvailableCaptainId(*this, npc.settlementId, npc.pos);
        if (captainId == 0) continue;

        npc.warCaptainId = captainId;
        npc.warReady = true;
    }

    // Reset squad indexes
    for (auto& npc : npcs) {
        if (!npc.alive || npc.isDying) continue;
        if (npc.settlementId < 0) continue;
        if (npc.humanRole == NPC::HumanRole::CAPTAIN || npc.humanRole == NPC::HumanRole::WARRIOR) {
            npc.warSquadIndex = -1;
        }
    }

    int nextSquadIndex = 0;
    for (auto& captain : npcs) {
        if (!captain.alive || captain.isDying) continue;
        if (captain.humanRole != NPC::HumanRole::CAPTAIN) continue;
        if (captain.settlementId < 0) continue;

        int warriorCount = CountAssignedWarriorsForCaptain(*this, captain.settlementId, captain.id);
        if (warriorCount >= 2) {
            captain.warSquadIndex = nextSquadIndex++;

            for (auto& warrior : npcs) {
                if (!warrior.alive || warrior.isDying) continue;
                if (warrior.settlementId != captain.settlementId) continue;
                if (warrior.humanRole != NPC::HumanRole::WARRIOR) continue;
                if (warrior.warCaptainId == captain.id) {
                    warrior.warSquadIndex = captain.warSquadIndex;
                    warrior.warReady = true;
                }
            }
        }
    }
}

void World::UpdateSettlementWarPreparation(float dt)
{
    (void)dt;

    RefreshSettlementWarSquads();

    for (int sid = 0; sid < (int)settlements.size(); sid++) {
        Settlement& s = settlements[sid];
        if (!s.alive) continue;
        if (!s.warActive) continue;

        if (!IsSettlementAliveAndValid(s.warTargetSettlementId)) {
            s.preparedSquadCount = 0;
            s.offensiveWaveReady = false;
            continue;
        }

        s.preparedSquadCount = CountReadySquadsForSettlement(*this, sid);
        s.offensiveWaveReady = (s.preparedSquadCount >= 3);
    }
}

void World::UpdateSettlementWarAssignments()
{
    for (int sid = 0; sid < (int)settlements.size(); sid++) {
        Settlement& s = settlements[sid];
        if (!s.alive) continue;
        if (!s.warActive) continue;

        if (!IsSettlementAliveAndValid(s.warTargetSettlementId)) {
            StopSettlementWar(sid);
            continue;
        }

        if (!s.offensiveWaveReady) {
            s.attackWaveLaunched = false;
            s.warWaveSize = 0;
            continue;
        }

        // If an offensive wave is already alive, do not relaunch yet
        bool anyOffensiveAlive = false;
        for (const auto& npc : npcs) {
            if (!npc.alive || npc.isDying) continue;
            if (!npc.warAssigned) continue;
            if (npc.warFromSettlementId != sid) continue;
            if (npc.warTargetSettlementId != s.warTargetSettlementId) continue;
            if (npc.warIsDefender) continue;
            anyOffensiveAlive = true;
            break;
        }

        if (anyOffensiveAlive) {
            s.attackWaveLaunched = true;
            continue;
        }

        // Clean stale offensive flags before launching a new wave
        for (auto& npc : npcs) {
            if (!npc.alive || npc.isDying) continue;
            if (!npc.warAssigned) continue;
            if (npc.warFromSettlementId != sid) continue;
            if (npc.warIsDefender) continue;

            npc.warAssigned = false;
            npc.warMarching = false;
            npc.warTargetSettlementId = -1;
            npc.warTargetPos = {0.0f, 0.0f};
        }

        int launchedSquads = 0;
        int launchedUnits = 0;

        for (auto& captain : npcs) {
            if (!captain.alive || captain.isDying) continue;
            if (captain.settlementId != sid) continue;
            if (captain.humanRole != NPC::HumanRole::CAPTAIN) continue;
            if (captain.warSquadIndex < 0) continue;

            int warriorCount = CountAssignedWarriorsForCaptain(*this, sid, captain.id);
            if (warriorCount < 2) continue;

            captain.warAssigned = true;
            captain.warFromSettlementId = sid;
            captain.warTargetSettlementId = s.warTargetSettlementId;
            captain.warMarching = true;
            captain.warTargetPos = settlements[s.warTargetSettlementId].centerPx;
            captain.warIsDefender = false;
            captain.warReady = true;

            // Settlement war must not be blocked by stale player/manual state
            captain.manualControl = false;
            captain.hasMoveTarget = false;
            captain.captainHasMoveOrder = false;
            captain.captainHasAttackOrder = false;
            captain.captainAttackGroupId = -1;
            captain.captainAttackTargetId = 0;

            launchedUnits++;

            int assignedToCaptain = 0;
            for (auto& warrior : npcs) {
                if (!warrior.alive || warrior.isDying) continue;
                if (warrior.settlementId != sid) continue;
                if (warrior.humanRole != NPC::HumanRole::WARRIOR) continue;
                if (warrior.warCaptainId != captain.id) continue;

                warrior.warAssigned = true;
                warrior.warFromSettlementId = sid;
                warrior.warTargetSettlementId = s.warTargetSettlementId;
                warrior.warMarching = true;
                warrior.warTargetPos = settlements[s.warTargetSettlementId].centerPx;
                warrior.warIsDefender = false;
                warrior.warReady = true;

                assignedToCaptain++;
                launchedUnits++;

                if (assignedToCaptain >= 5) break;
            }

            launchedSquads++;
            if (launchedSquads >= 3) break;
        }

        if (launchedSquads >= 3) {
            s.attackWaveLaunched = true;
            s.warWaveSize = launchedUnits;
        } else {
            // failed launch: fully roll back offensive assignment for this settlement
            for (auto& npc : npcs) {
                if (!npc.alive || npc.isDying) continue;
                if (!npc.warAssigned) continue;
                if (npc.warFromSettlementId != sid) continue;
                if (npc.warIsDefender) continue;

                npc.warAssigned = false;
                npc.warMarching = false;
                npc.warTargetSettlementId = -1;
                npc.warTargetPos = {0.0f, 0.0f};
            }
            s.attackWaveLaunched = false;
            s.warWaveSize = 0;
        }
    }
}

void World::UpdateSettlementDefense(float dt)
{
    (void)dt;

    RefreshSettlementWarSquads();

    for (int sid = 0; sid < (int)settlements.size(); sid++) {
        Settlement& s = settlements[sid];
        if (!s.alive) continue;
        if (!s.warActive) continue;

        bool underAttack = IsEnemySettlementTroopNearSettlement(*this, sid, CELL_SIZE * 20.0f);
        s.defensiveMobilization = underAttack;

        if (!underAttack) {
            // Release only defender state when danger is gone
            for (auto& npc : npcs) {
                if (!npc.alive || npc.isDying) continue;
                if (!npc.warAssigned) continue;
                if (!npc.warIsDefender) continue;
                if (npc.warFromSettlementId != sid) continue;

                npc.warAssigned = false;
                npc.warMarching = false;
                npc.warTargetSettlementId = -1;
                npc.warTargetPos = {0.0f, 0.0f};
                npc.warIsDefender = false;
                npc.warInBattle = false;
                npc.warBattleLockTimer = 0.0f;
            }
            continue;
        }

        int hostileIndex = FindNearestHostileTroopNearSettlement(*this, sid, s.centerPx, CELL_SIZE * 20.0f);
        if (hostileIndex == -1) continue;

        int targetEnemySettlement = npcs[hostileIndex].settlementId;
        if (targetEnemySettlement < 0) continue;

        // Mobilize all available captains first
        for (auto& captain : npcs) {
            if (!captain.alive || captain.isDying) continue;
            if (captain.settlementId != sid) continue;
            if (captain.humanRole != NPC::HumanRole::CAPTAIN) continue;

            captain.warAssigned = true;
            captain.warFromSettlementId = sid;
            captain.warTargetSettlementId = targetEnemySettlement;
            captain.warMarching = false;
            captain.warTargetPos = s.centerPx;
            captain.warIsDefender = true;
            captain.warReady = true;

            // Settlement defense must not be blocked by stale player/manual state
            captain.manualControl = false;
            captain.hasMoveTarget = false;
            captain.captainHasMoveOrder = false;
            captain.captainHasAttackOrder = false;
            captain.captainAttackGroupId = -1;
            captain.captainAttackTargetId = 0;
        }

        // Mobilize all available warriors too, even if captain link is absent or broken
        for (auto& warrior : npcs) {
            if (!warrior.alive || warrior.isDying) continue;
            if (warrior.settlementId != sid) continue;
            if (warrior.humanRole != NPC::HumanRole::WARRIOR) continue;

            warrior.warAssigned = true;
            warrior.warFromSettlementId = sid;
            warrior.warTargetSettlementId = targetEnemySettlement;
            warrior.warMarching = false;
            warrior.warTargetPos = s.centerPx;
            warrior.warIsDefender = true;
            warrior.warReady = true;
        }
    }
}

void World::UpdateSettlementWars(float dt)
{
    UpdateSettlementWarPreparation(dt);
    UpdateSettlementDefense(dt);
    const float defenseRadius = CELL_SIZE * 10.0f;

    for (auto& npc : npcs) {
        if (!npc.alive || npc.isDying) continue;

        if (npc.warAssigned) {
            int localEnemyCombatCount = CountEnemyCombatUnitsNearNpc(*this, npc, CELL_SIZE * 8.0f);

            if (localEnemyCombatCount > 0) {
                npc.warInBattle = true;
                npc.warBattleLockTimer = 1.5f;
            } else if (npc.warBattleLockTimer > 0.0f) {
                npc.warBattleLockTimer -= dt;
                if (npc.warBattleLockTimer <= 0.0f) {
                    npc.warBattleLockTimer = 0.0f;
                    npc.warInBattle = false;
                }
            } else {
                npc.warInBattle = false;
            }
        } else {
            npc.warInBattle = false;
            npc.warBattleLockTimer = 0.0f;
        }
    }

    // Release stale defender assignments if no hostile troops remain near their home settlement
    for (auto& npc : npcs) {
        if (!npc.alive || npc.isDying) continue;
        if (!npc.warAssigned) continue;
        if (!npc.warIsDefender) continue;
        if (npc.settlementId < 0 || npc.settlementId >= (int)settlements.size()) continue;
        if (!settlements[npc.settlementId].alive) continue;

        bool stillUnderAttack = IsEnemySettlementTroopNearSettlement(*this, npc.settlementId, defenseRadius);
        if (!stillUnderAttack) {
            npc.warAssigned = false;
            npc.warMarching = false;
            npc.warTargetSettlementId = -1;
            npc.warTargetPos = {0.0f, 0.0f};
            npc.warIsDefender = false;
            npc.warInBattle = false;
            npc.warBattleLockTimer = 0.0f;
        }
    }

    for (int sid = 0; sid < (int)settlements.size(); sid++) {
        Settlement& s = settlements[sid];
        if (!s.alive) continue;
        if (!s.warActive) continue;

        if (!IsSettlementAliveAndValid(s.warTargetSettlementId)) {
            StopSettlementWar(sid);
            continue;
        }

        bool anyOffensiveAssignedAlive = false;
        for (const auto& npc : npcs) {
            if (!npc.alive || npc.isDying) continue;
            if (!npc.warAssigned) continue;
            if (npc.warFromSettlementId != sid) continue;
            if (npc.warTargetSettlementId != s.warTargetSettlementId) continue;
            if (npc.warIsDefender) continue;

            anyOffensiveAssignedAlive = true;
            break;
        }

        if (!anyOffensiveAssignedAlive) {
            s.attackWaveLaunched = false;
            s.warWaveSize = 0;
        }
    }

    // Preparation and launch must happen every frame after stale-wave cleanup
    UpdateSettlementWarPreparation(dt);
    UpdateSettlementWarAssignments();
}

void World::BeginNpcAttack(NPC& npc, Vector2 targetPos) {
    Vector2 dir = Vector2Subtract(targetPos, npc.pos);

    if (Vector2Length(dir) <= 0.001f) {
        if (Vector2Length(npc.vel) > 0.001f) {
            dir = Vector2Normalize(npc.vel);
        } else {
            dir = {0.0f, 1.0f};
        }
    } else {
        dir = Vector2Normalize(dir);
    }

    npc.isAttacking = true;
    npc.attackAnimTimer = npc.attackAnimDuration;
    npc.attackAnimDir = dir;
}

void World::BeginNpcDeath(NPC& npc) {
    if (!npc.alive || npc.isDying) return;

    npc.alive = false;
    npc.isDying = true;
    npc.deathTimer = 0.0f;
    npc.hp = 0.0f;
    npc.vel = {0.0f, 0.0f};
    npc.attackCooldown = 0.0f;
    npc.inCombat = false;

    npc.manualControl = false;
    npc.hasMoveTarget = false;
    npc.captainHasMoveOrder = false;
    npc.captainHasAttackOrder = false;
    npc.captainAttackGroupId = -1;
    npc.captainAttackTargetId = 0;
    npc.warAssigned = false;
    npc.warFromSettlementId = -1;
    npc.warTargetSettlementId = -1;
    npc.warMarching = false;
    npc.warTargetPos = {0.0f, 0.0f};
    npc.warCaptainId = 0;
    npc.warSquadIndex = -1;
    npc.warIsDefender = false;
    npc.warReady = false;
    npc.warInBattle = false;
    npc.warBattleLockTimer = 0.0f;

    if (selectedCaptainId == npc.id) {
        selectedCaptainId = 0;
    }

    if (npc.humanRole == NPC::HumanRole::CAPTAIN) {
        for (auto& other : npcs) {
            if (other.leaderCaptainId == npc.id) {
                other.leaderCaptainId = 0;
                other.formationSlot = -1;

                // During settlement war, followers must keep fighting and not fall back to idle/home behavior
                if (!other.warAssigned) {
                    other.inCombat = false;
                }
            }

            // If the dead captain was the warrior's war captain, detach only the captain reference,
            // but keep the warrior in war state so he continues fighting
            if (other.warCaptainId == npc.id) {
                other.warCaptainId = 0;
                other.warReady = true;
                other.warInBattle = true;
                other.warBattleLockTimer = 1.5f;
            }
        }
    }

    npc.leaderCaptainId = 0;
    npc.formationSlot = -1;
}

void World::IssueCaptainMoveOrder(uint32_t captainId, Vector2 targetPx) {
    NPC* cap = FindNpcById(captainId);
    if (!cap) return;
    if (cap->humanRole != NPC::HumanRole::CAPTAIN) return;

    selectedCaptainId = captainId;

    cap->captainAutoMode = false;
    cap->captainHasMoveOrder = true;
    cap->captainMoveTarget = targetPx;

    cap->captainHasAttackOrder = false;
    cap->captainAttackGroupId = -1;
    cap->captainAttackTargetId = 0;

    // Keep older captain command fields synchronized
    cap->manualControl = true;
    cap->hasMoveTarget = true;
    cap->moveTargetPx = targetPx;
}

static void DrawBanditTriangle(Vector2 pos, float size, Color color) {
    Vector2 p1 = {pos.x, pos.y + size};
    Vector2 p2 = {pos.x + size, pos.y - size};
    Vector2 p3 = {pos.x - size, pos.y - size};
    DrawTriangle(p1, p2, p3, color);
    DrawTriangleLines(p1, p2, p3, BLACK);
}

// Initializes world state and runtime resources
void World::Init()
{
    cols = worldW / CELL_SIZE;
    rows = worldH / CELL_SIZE;

    terrain = Terrain(cols, rows, worldSeed);
    terrain.generate();

    settlements.clear();
    npcs.clear();


    plants.clear();
    animals.clear();

    banditSpawnTimer = 0.0f;
    nextBanditGroupId = 1;

    LoadNpcSprites();

    LoadFireSprites();
    LoadBarracksSprite();
    UpdateCampfires();
    settlements.clear();
    npcs.clear();
    nextNpcId = 1;
    selectedCaptainId = 0;

    GenerateNature(2000, 20);
}
void World::Shutdown()
{
    UnloadNpcSprites();
    UnloadFireSprites();
    UnloadBarracksSprite();
}



// Updates the world simulation for one frame
void World::Update(float dt) {

    // Spawn new bandit groups
    banditSpawnTimer -= dt;

    if (banditSpawnTimer <= 0.0f) {
        banditSpawnTimer = 45.0f;

        int count = GetRandomValue(5, 8);
        Vector2 spawnPos = RandomOutsideSpawn(worldW, worldH);

        Vector2 toWorldCenter = {
                worldW * 0.5f - spawnPos.x,
                worldH * 0.5f - spawnPos.y
        };
        Vector2 dir = SafeNormalize(toWorldCenter);
        int gid = nextBanditGroupId++;

        for (int i = 0; i < count; i++) {
            NPC npc;
            npc.id = nextNpcId++;
            npc.type = NPC::Type::HUMAN;
            npc.humanRole = NPC::HumanRole::BANDIT;
            npc.skinId = (uint16_t)GetRandomValue(0, 2);
            npc.settlementId = -1;

            npc.banditGroupId = gid;
            npc.banditGroupDir = dir;

            npc.speed = 40.0f;
            npc.hp = 140.0f;
            npc.damage = 14.0f;
            npc.pos = {
                    spawnPos.x + (float)GetRandomValue(-10, 10),
                    spawnPos.y + (float)GetRandomValue(-10, 10)
            };
            npc.vel = {dir.x * npc.speed, dir.y * npc.speed};

            ClampNpcInsideWorld(npc, worldW, worldH);
            npcs.push_back(npc);
        }
    }

    // Update NPC behavior
    for (auto& npc : npcs) {
        if (npc.isDying) {
            npc.deathTimer += dt;
            if (npc.deathTimer > npc.deathDuration) {
                npc.deathTimer = npc.deathDuration;
            }
            continue;
        }

        if (npc.attackAnimTimer > 0.0f) {
            npc.attackAnimTimer -= dt;
            if (npc.attackAnimTimer <= 0.0f) {
                npc.attackAnimTimer = 0.0f;
                npc.isAttacking = false;
            }
        } else {
            npc.isAttacking = false;
        }

        if (npc.type == NPC::Type::HUMAN && npc.alive) {
            HumanBehavior::Update(*this, npc, dt);
        }

        if (npc.alive) {
            ClampNpcInsideWorld(npc, worldW, worldH);
        }
    }

    // Advance fire animation
    fireAnimT += dt;
    if (fireAnimT >= fireAnimSpeed) {
        fireAnimT = 0.0f;
        fireFrame = (fireFrame + 1) % FIRE_FRAMES;
    }

    UpdateCampfires();
    UpdateBarracks();
    UpdateBarracksProduction(dt);

    // Bind wild humans to the first settlement they enter
    for (auto& npc : npcs) {
        if (!npc.alive || npc.isDying) continue;
        if (npc.settlementId != -1) continue;

        if (npc.humanRole != NPC::HumanRole::CIVILIAN &&
            npc.humanRole != NPC::HumanRole::WARRIOR &&
            npc.humanRole != NPC::HumanRole::CAPTAIN) {
            continue;
        }

        for (int i = 0; i < (int)settlements.size(); i++) {
            if (!settlements[i].alive) continue;
            if (PointInSettlementPx(settlements[i], npc.pos)) {
                npc.settlementId = i;
                break;
            }
        }
    }

    // Remove entities whose death state has fully finished
    npcs.erase(
            std::remove_if(npcs.begin(), npcs.end(),
                           [](const NPC& n) {
                               return !n.alive && (!n.isDying || n.deathTimer >= n.deathDuration);
                           }),
            npcs.end()
    );

    for (auto& s : settlements) {
        if (!s.alive) continue;

        bool anyoneLeft = false;
        for (const auto& npc : npcs) {
            if (!npc.alive || npc.isDying) continue;
            if (npc.settlementId == (&s - &settlements[0]) &&
                npc.humanRole != NPC::HumanRole::BANDIT) {
                anyoneLeft = true;
                break;
            }
        }

        if (!anyoneLeft) {
            int settlementIndex = (int)(&s - &settlements[0]);
            if (s.warActive) {
                StopSettlementWar(settlementIndex);
            }
            s.alive = false;
        }
    }
    for (auto& plant : plants) {
        plant.Update(dt, &terrain);
    }
    for (auto& animal : animals) {
        animal->Update(dt, &terrain);
    }

    MergeSettlementsIfNeeded();
    UpdateCampfires();
    UpdateBarracks();
    UpdateSettlementWars(dt);
}

void World::SpawnAnimal(Vector2 pos) {
    animals.push_back(std::make_unique<Animal>(pos));
}

void World::SpawnPlant(Vector2 pos, float treeChance) {
    plants.push_back(Plant(pos, treeChance));
}

void World::GenerateNature(int plantCount, int animalCount) {
    const auto& biomes = terrain.getBiomes();
    
    for (int i = 0; i < plantCount; i++) {
        Vector2 pos = { (float)GetRandomValue(0, worldW), (float)GetRandomValue(0, worldH) };
        
        const Biome* biome = terrain.getBiomeAt(pos.x, pos.y);
        if (biome && biome->props.canBuild) {
            if (biome->props.hasVegetation || biome->props.treeChance > 0.0f) {
                SpawnPlant(pos, biome->props.treeChance);
            }
        }
    }
    
    for (int i = 0; i < animalCount; i++) {
        Vector2 pos = { (float)GetRandomValue(0, worldW), (float)GetRandomValue(0, worldH) };
        
        const Biome* biome = terrain.getBiomeAt(pos.x, pos.y);
        if (biome && !biome->props.isWater && biome->props.canWalk) {
            SpawnAnimal(pos);
        }
    }
}


// Returns a fully opaque settlement color
Color GetSafeSettlementColor(const World &w, int sid) {
    Color c = {220, 220, 220, 255};

    if (sid >= 0 && sid < (int)w.settlements.size() && w.settlements[sid].alive) {
        c = w.settlements[sid].color;
    }

    c.a = 255;
    return c;
}

// Draws a solid diamond marker
static void DrawDiamondSolid(Vector2 center, int r, Color col)
{
    col.a = 255;

    int cx = (int)center.x;
    int cy = (int)center.y;

    for (int dy = -r; dy <= 0; dy++) {
        int half = r + dy;
        DrawLine(cx - half, cy + dy, cx + half, cy + dy, col);
    }

    for (int dy = 1; dy <= r; dy++) {
        int half = r - dy;
        DrawLine(cx - half, cy + dy, cx + half, cy + dy, col);
    }
}

// Draws a diamond outline marker
static void DrawDiamondOutline(Vector2 center, int r, Color col)
{
    col.a = 255;
    int cx = (int)center.x;
    int cy = (int)center.y;

    Vector2 top    = {(float)cx,     (float)(cy - r)};
    Vector2 right  = {(float)(cx+r), (float)cy};
    Vector2 bottom = {(float)cx,     (float)(cy + r)};
    Vector2 left   = {(float)(cx-r), (float)cy};

    DrawLineV(top, right, col);
    DrawLineV(right, bottom, col);
    DrawLineV(bottom, left, col);
    DrawLineV(left, top, col);
}

void World::Draw() const {

    terrain.draw();
    for (const auto& plant : plants) {
        plant.Draw();
    }
    for (const auto& animal : animals) {
        animal->Draw();
    }

    // settlements
    for (const auto& s : settlements) {
        if (!s.alive) continue;

        Color fill = Fade(s.color, 0.25f);

        for (int tile : s.tiles) {
            int cx = tile % cols;
            int cy = tile / cols;

            DrawRectangle(
                    cx * CELL_SIZE,
                    cy * CELL_SIZE,
                    CELL_SIZE,
                    CELL_SIZE,
                    fill
            );
        }
    }

    for (int i = 0; i < (int)settlements.size(); i++) {
        const Settlement& s = settlements[i];
        if (!s.alive) continue;
        if (!s.warActive) continue;

        Color c = s.offensiveWaveReady ? Color{220,60,60,255} : Color{220,190,60,255};
        DrawRectangleLinesEx(s.boundsPx, 2.0f, c);

        if (s.defensiveMobilization) {
            DrawCircleV(s.centerPx, 6.0f, Color{255,140,60,220});
        }

        if (s.warTargetSettlementId >= 0 &&
            s.warTargetSettlementId < (int)settlements.size() &&
            settlements[s.warTargetSettlementId].alive) {
            DrawLineV(s.centerPx, settlements[s.warTargetSettlementId].centerPx, Color{220, 60, 60, 180});
        }
    }

    // Draw all barracks
    for (const auto& s : settlements) {
        if (!s.alive) continue;

        for (const auto& b : s.barracksList) {
            if (!b.alive) continue;

            float w = (float)CELL_SIZE * 8.0f;
            float h = (float)CELL_SIZE * 8.0f;

            if (barracksTexLoaded && barracksTex.id != 0) {
                Rectangle src{0, 0, (float)barracksTex.width, (float)barracksTex.height};
                Rectangle dst{
                        floorf(b.posPx.x - w * 0.5f),
                        floorf(b.posPx.y - h * 0.92f),
                        w,
                        h
                };
                DrawTexturePro(barracksTex, src, dst, Vector2{0,0}, 0.0f, WHITE);
            } else {
                Rectangle base{
                        floorf(b.posPx.x - w * 0.5f),
                        floorf(b.posPx.y - h * 0.82f),
                        w, h
                };
                DrawRectangleRec(base, Color{110, 80, 45, 255});
                DrawRectangleLinesEx(base, 1.0f, BLACK);
                DrawRectangle((int)(base.x + w * 0.30f), (int)(base.y + h * 0.55f),
                              (int)(w * 0.40f), (int)(h * 0.25f), Color{70, 45, 20, 255});
            }

            if (b.maxHp > 0.0f) {
                float hpRatio = b.hp / b.maxHp;
                if (hpRatio < 0.0f) hpRatio = 0.0f;
                if (hpRatio > 1.0f) hpRatio = 1.0f;

                float barW = w * 0.8f;
                float barH = 4.0f;
                float barX = floorf(b.posPx.x - barW * 0.5f);
                float barY = floorf(b.posPx.y - h * 0.95f);

                DrawRectangle((int)barX, (int)barY, (int)barW, (int)barH, Color{40, 20, 20, 220});

                int fillW = (int)floorf(barW * hpRatio);
                if (fillW > 0) {
                    DrawRectangle((int)barX, (int)barY, fillW, (int)barH, Color{210, 70, 70, 255});
                }
            }
        }
    }

    // Draw NPCs at a fixed world scale
    for (const auto& npc : npcs) {

        if (!npc.alive && !npc.isDying) continue;

        int v = (int)(npc.skinId % NPC_VARIANTS);

        const Texture2D* tex = nullptr;
        bool loaded = false;


        switch (npc.humanRole) {
            case NPC::HumanRole::CIVILIAN:
                tex = &npcTexCivilian[v];
                loaded = npcTexCivilianLoaded[v];
                break;
            case NPC::HumanRole::WARRIOR:
                tex = &npcTexWarrior[v];
                loaded = npcTexWarriorLoaded[v];
                break;
            case NPC::HumanRole::BANDIT:
                tex = &npcTexBandit[v];
                loaded = npcTexBanditLoaded[v];
                break;
            case NPC::HumanRole::CAPTAIN:
                tex = &npcTexCaptain[v];
                loaded = npcTexCaptainLoaded[v];
                break;
            default:
                break;
        }
        float mult = 1.0f;
        if (npc.humanRole == NPC::HumanRole::CIVILIAN) mult = 1.15f;
        if (npc.humanRole == NPC::HumanRole::BANDIT)   mult = 1.05f;
        if (npc.humanRole == NPC::HumanRole::CAPTAIN)  mult = 1.6f;

        float w = (float)CELL_SIZE * 2.0f * mult;
        float h = (float)CELL_SIZE * 2.0f * mult;

        Rectangle dst{
                floorf(npc.pos.x - w * 0.5f),
                floorf(npc.pos.y - h * 0.5f),
                w, h
        };

        if (!tex || !loaded || tex->id == 0) {
            float size = CELL_SIZE * 0.4f;
            Color c = GetSafeSettlementColor(*this, npc.settlementId);
            Vector2 drawPos = npc.pos;

            if (npc.isDying) {
                float t = Clamp(npc.deathTimer / npc.deathDuration, 0.0f, 1.0f);
                c.a = (unsigned char)(255.0f * (1.0f - 0.70f * t));
            }

            if (npc.attackAnimTimer > 0.0f && !npc.isDying) {
                float t = 1.0f - (npc.attackAnimTimer / npc.attackAnimDuration);
                t = Clamp(t, 0.0f, 1.0f);
                float pulse = sinf(t * PI);
                float push = 6.0f * pulse;
                drawPos.x += npc.attackAnimDir.x * push;
                drawPos.y += npc.attackAnimDir.y * push;
            }

            switch (npc.humanRole) {
                case NPC::HumanRole::CIVILIAN:
                    DrawCircleV(drawPos, size, c);
                    break;
                case NPC::HumanRole::WARRIOR:
                    DrawRectangle(drawPos.x-size, drawPos.y-size, size*2, size*2, c);
                    break;
                case NPC::HumanRole::BANDIT: {
                    Color banditCol = Color{160,80,200,255};
                    if (npc.isDying) {
                        float t = Clamp(npc.deathTimer / npc.deathDuration, 0.0f, 1.0f);
                        banditCol.a = (unsigned char)(255.0f * (1.0f - 0.70f * t));
                    }
                    DrawTriangle(
                            {drawPos.x, drawPos.y + CELL_SIZE*0.7f},
                            {drawPos.x + CELL_SIZE*0.7f, drawPos.y - CELL_SIZE*0.7f},
                            {drawPos.x - CELL_SIZE*0.7f, drawPos.y - CELL_SIZE*0.7f},
                            banditCol);
                    break;
                }
                default:
                    break;
            }
            continue;
        }

        Rectangle src{ 0.0f, 0.0f, (float)tex->width, (float)tex->height };

        Rectangle drawDst = dst;
        Color tint = WHITE;

        if (npc.isDying) {
            float t = Clamp(npc.deathTimer / npc.deathDuration, 0.0f, 1.0f);

            drawDst.y += floorf(10.0f * t);
            drawDst.x -= floorf(w * 0.06f * t);
            drawDst.width = w * (1.0f + 0.12f * t);
            drawDst.height = h * (1.0f - 0.55f * t);

            tint.a = (unsigned char)(255.0f * (1.0f - 0.70f * t));
        }

        if (npc.attackAnimTimer > 0.0f && !npc.isDying) {
            float t = 1.0f - (npc.attackAnimTimer / npc.attackAnimDuration);
            t = Clamp(t, 0.0f, 1.0f);
            float pulse = sinf(t * PI);

            float push = 6.0f * pulse;
            drawDst.x += npc.attackAnimDir.x * push;
            drawDst.y += npc.attackAnimDir.y * push;

            drawDst.x -= (drawDst.width * 0.04f * pulse);
            drawDst.width *= (1.0f + 0.08f * pulse);
            drawDst.height *= (1.0f - 0.06f * pulse);
        }

        DrawTexturePro(*tex, src, drawDst, Vector2{0,0}, 0.0f, tint);
        if (npc.humanRole == NPC::HumanRole::CAPTAIN && npc.id == selectedCaptainId) {
            DrawCircleLines((int)npc.pos.x, (int)npc.pos.y, CELL_SIZE * 1.3f, YELLOW);
            DrawCircleLines((int)npc.pos.x, (int)npc.pos.y, CELL_SIZE * 1.3f + 1.0f, BLACK);
        }

        // Draw a settlement marker above the NPC
        if (npc.settlementId != -1) {
            Color sc = GetSafeSettlementColor(*this, npc.settlementId);
            sc.a = 255;

            Vector2 c = {
                    (float)((int)npc.pos.x),
                    (float)((int)(npc.pos.y - h - 8.0f))
            };

            const int r = 3;
            DrawDiamondSolid(c, r, sc);
            DrawDiamondOutline(c, r, BLACK);
        }
    }

    // Draw campfires
    int f = fireFrame;
    if (f < 0 || f >= FIRE_FRAMES) f = 0;

    for (const auto& s : settlements) {
        if (!s.alive) continue;
        if (!fireLoaded[f] || fireTex[f].id == 0) continue;

        float w = (float)CELL_SIZE * 4.0f;
        float h = (float)CELL_SIZE * 4.0f;

        Rectangle src{0,0,(float)fireTex[f].width,(float)fireTex[f].height};
        Rectangle dst{
                floorf(s.campfirePosPx.x - w*0.5f),
                floorf(s.campfirePosPx.y - h*0.5f),
                w, h
        };


        DrawTexturePro(fireTex[f], src, dst, {0,0}, 0.0f, WHITE);
    }
}

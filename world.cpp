#include "world.h"
#include "human_behavior.h"
#include "bandit_behavior.h"
#include "settlement.h"
#include "npc.h"
#include <algorithm>

// ------------------------------------------------------------
// Helpers
static Vector2 RandomOutsideSpawn(int w, int h) {
    const float margin = 80.0f;

    int side = GetRandomValue(0, 3);
    switch (side) {
        case 0: return { -margin, (float)GetRandomValue(0, h) };          // left
        case 1: return { (float)w + margin, (float)GetRandomValue(0, h) };// right
        case 2: return { (float)GetRandomValue(0, w), -margin };          // top
        default:return { (float)GetRandomValue(0, w), (float)h + margin };// bottom
    }
}
// ------------------------------------------------------------

static Vector2 RandomEdgeSpawn(int w, int h) {
    int side = GetRandomValue(0, 3);
    switch (side) {
        case 0: return { 0.0f, (float)GetRandomValue(0, h) };        // left
        case 1: return { (float)w, (float)GetRandomValue(0, h) };    // right
        case 2: return { (float)GetRandomValue(0, w), 0.0f };        // top
        default:return { (float)GetRandomValue(0, w), (float)h };   // bottom
    }
}

static void DrawBanditTriangle(Vector2 pos, float size, Color color) {
    Vector2 p1 = { pos.x, pos.y + size };
    Vector2 p2 = { pos.x + size, pos.y - size };
    Vector2 p3 = { pos.x - size, pos.y - size };
    DrawTriangle(p1, p2, p3, color);
    DrawTriangleLines(p1, p2, p3, BLACK);
}

// ------------------------------------------------------------
// Init
// ------------------------------------------------------------

void World::Init() {
    cols = worldW / CELL_SIZE;
    rows = worldH / CELL_SIZE;

    settlements.clear();
    npcs.clear();

    banditSpawnTimer = 15.0f;
    nextBanditGroupId = 1;

    // --- settlements ---
    {
        Settlement s; s.color = RED;
        s.zones.push_back({20,15,28,10});
        s.zones.push_back({20,25,10,18});
        s.centerPx = ComputeSettlementCenterPx(s);
        settlements.push_back(s);
    }
    {
        Settlement s; s.color = BLUE;
        s.zones.push_back({90,18,30,10});
        s.zones.push_back({102,28,6,22});
        s.centerPx = ComputeSettlementCenterPx(s);
        settlements.push_back(s);
    }
    {
        Settlement s; s.color = GREEN;
        s.zones.push_back({55,70,26,14});
        s.zones.push_back({65,60,12,10});
        s.centerPx = ComputeSettlementCenterPx(s);
        settlements.push_back(s);
    }

    // --- spawn ONLY civilians + warriors ---
    auto RandomCellInsideSettlement = [&](const Settlement& s) -> Vector2 {
        const Rectangle& r = s.zones[GetRandomValue(0, (int)s.zones.size() - 1)];
        int cx = GetRandomValue((int)r.x, (int)r.x + (int)r.width - 1);
        int cy = GetRandomValue((int)r.y, (int)r.y + (int)r.height - 1);
        return CellToPxCenter(cx, cy);
    };

    for (int i = 0; i < 60; i++) {
        int sid = GetRandomValue(0, (int)settlements.size() - 1);
        const Settlement& s = settlements[sid];

        NPC npc;
        npc.type = NPC::Type::HUMAN;
        npc.settlementId = sid;

        int roll = GetRandomValue(0, 99);
        npc.humanRole = (roll < 60)
            ? NPC::HumanRole::CIVILIAN
            : NPC::HumanRole::WARRIOR;

        npc.speed = (npc.humanRole == NPC::HumanRole::WARRIOR) ? 7.0f : 5.0f;
        npc.pos = RandomCellInsideSettlement(s);

        Vector2 dir = SafeNormalize(RandomUnit2D());


        npcs.push_back(npc);
    }
}

// ------------------------------------------------------------
// Update
// ------------------------------------------------------------

void World::Update(float dt) {
    // -------- bandit group spawning (DEBUG: often) --------
    banditSpawnTimer -= dt;


    if (banditSpawnTimer <= 0.0f) {
        banditSpawnTimer = 45.0f; // frequent for debug

        int count = GetRandomValue(5, 8);
        Vector2 spawnPos = RandomOutsideSpawn(worldW, worldH);

// направление ВСЕГДА внутрь карты
        Vector2 toWorldCenter = {
                worldW * 0.5f - spawnPos.x,
                worldH * 0.5f - spawnPos.y
        };
        Vector2 dir = SafeNormalize(toWorldCenter);
        int gid = nextBanditGroupId++;

        for (int i = 0; i < count; i++) {
            NPC npc;
            npc.type = NPC::Type::HUMAN;
            npc.humanRole = NPC::HumanRole::BANDIT;
            npc.settlementId = -1;

            npc.banditGroupId = gid;
            npc.banditGroupDir = dir;

            npc.speed = 40.0f;
            npc.pos = {
                spawnPos.x + GetRandomValue(-10, 10),
                spawnPos.y + GetRandomValue(-10, 10)
            };
            npc.vel = { dir.x * npc.speed, dir.y * npc.speed };

            npcs.push_back(npc);
        }
    }

    // -------- update all NPCs --------
    for (auto& npc : npcs) {
        if (npc.type == NPC::Type::HUMAN) {
            HumanBehavior::Update(*this, npc, dt);
        }
    }

    // -------- cleanup bandits outside map --------
    npcs.erase(
        std::remove_if(npcs.begin(), npcs.end(),
            [&](const NPC& n) {
                return n.humanRole == NPC::HumanRole::BANDIT &&
                       (n.pos.x < -100 || n.pos.y < -100 ||
                        n.pos.x > worldW + 100 || n.pos.y > worldH + 100);
            }),
        npcs.end()
    );
    npcs.erase(
            std::remove_if(npcs.begin(), npcs.end(),
                           [](const NPC& n) { return !n.alive; }),
            npcs.end()
    );
    for (auto& s : settlements) {
        if (!s.alive) continue;

        bool anyoneLeft = false;
        for (const auto& npc : npcs) {
            if (!npc.alive) continue;
            if (npc.settlementId == (&s - &settlements[0]) &&
                npc.humanRole != NPC::HumanRole::BANDIT) {
                anyoneLeft = true;
                break;
            }
        }

        if (!anyoneLeft) {
            s.alive = false;
        }
    }
}

// ------------------------------------------------------------
// Draw
// ------------------------------------------------------------

void World::Draw() const {
    // grass
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            Color base = ((x + y) % 2 == 0)
                ? Color{60,120,60,255}
                : Color{55,110,55,255};
            DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, base);
        }
    }

    // settlements
    for (const auto& s : settlements) {
        Color fill = Fade(s.color, 0.25f);
        for (const auto& r : s.zones) {
            for (int cy = (int)r.y; cy < (int)r.y + (int)r.height; cy++) {
                for (int cx = (int)r.x; cx < (int)r.x + (int)r.width; cx++) {
                    if (cx < 0 || cx >= cols || cy < 0 || cy >= rows) continue;
                    DrawRectangle(cx * CELL_SIZE, cy * CELL_SIZE, CELL_SIZE, CELL_SIZE, fill);
                }
            }
            DrawRectangleLines(
                (int)r.x * CELL_SIZE,
                (int)r.y * CELL_SIZE,
                (int)r.width * CELL_SIZE,
                (int)r.height * CELL_SIZE,
                s.color
            );
        }
        DrawCircleV(s.centerPx, 3, s.color);
    }

    // NPCs
    for (const auto& npc : npcs) {
        float size = CELL_SIZE * 0.4f;

        switch (npc.humanRole) {
            case NPC::HumanRole::CIVILIAN:
                DrawCircleV(npc.pos, size, settlements[npc.settlementId].color);
                break;

            case NPC::HumanRole::WARRIOR:
                DrawRectangle(
                    npc.pos.x - size,
                    npc.pos.y - size,
                    size * 2,
                    size * 2,
                    settlements[npc.settlementId].color
                );
                break;

            case NPC::HumanRole::BANDIT:
                DrawBanditTriangle(
                    npc.pos,
                    CELL_SIZE * 0.7f,
                    Color{160, 80, 200, 255}
                );
                break;
        }
    }
}
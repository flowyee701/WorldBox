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
        case 0:
            return {-margin, (float) GetRandomValue(0, h)};          // left
        case 1:
            return {(float) w + margin, (float) GetRandomValue(0, h)};// right
        case 2:
            return {(float) GetRandomValue(0, w), -margin};          // top
        default:
            return {(float) GetRandomValue(0, w), (float) h + margin};// bottom
    }
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
bool World::PointInSettlementPx(const Settlement& s, Vector2 pos) const {
    int cx = (int)(pos.x / CELL_SIZE);
    int cy = (int)(pos.y / CELL_SIZE);

    if (cx < 0 || cy < 0 || cx >= cols || cy >= rows)
        return false;

    int tileId = cy * cols + cx;
    return s.tiles.find(tileId) != s.tiles.end();
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


// ------------------------------------------------------------
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

            // объединяем j -> i
            for (int tile : settlements[j].tiles)
                settlements[i].tiles.insert(tile);

            for (auto& npc : npcs)
                if (npc.settlementId == j)
                    npc.settlementId = i;

            settlements[j].tiles.clear();
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

    NPC npc;
    npc.type = NPC::Type::HUMAN;
    npc.humanRole = NPC::HumanRole::CIVILIAN;
    npc.pos = pos;
    npc.vel = {0, 0};
    npc.speed = 15.0f;
    npc.hp = 100.0f;
    npc.alive = true;
    npc.settlementId = -1;
    npc.damage = 0.0f;

    // 1) если клик внутри существующего поселения — просто присоединяем
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

    // 2) иначе — ищем рядом "свободных" жителей без поселения
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

        // если есть 2 рядом — создаём новое поселение (3-й будет текущий npc)
        if (nearbyFreeCivs.size() >= 2) {
            Settlement s;
            s.alive = true;

            int cx = (int)(pos.x / CELL_SIZE);
            int cy = (int)(pos.y / CELL_SIZE);

            constexpr int R = 8; // радиус поселения в клетках

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
    NPC npc;
    npc.type = NPC::Type::HUMAN;
    npc.humanRole = NPC::HumanRole::WARRIOR;
    npc.pos = pos;
    npc.vel = {0,0};
    npc.hp = 200.0f;
    npc.damage = 20.0f;
    npc.settlementId = -1;

    // 🔹 НОВОЕ: если клик внутри поселения — сразу привязываем
    for (int i = 0; i < (int)settlements.size(); i++) {
        if (!settlements[i].alive) continue;
        if (PointInSettlementPx(settlements[i], pos)) {
            npc.settlementId = i;
            break;
        }
    }

    npcs.push_back(npc);
}

static void DrawBanditTriangle(Vector2 pos, float size, Color color) {
    Vector2 p1 = {pos.x, pos.y + size};
    Vector2 p2 = {pos.x + size, pos.y - size};
    Vector2 p3 = {pos.x - size, pos.y - size};
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

    banditSpawnTimer = 0.0f;
    nextBanditGroupId = 1;
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
            npc.vel = {dir.x * npc.speed, dir.y * npc.speed};

            npcs.push_back(npc);
        }
    }

    // -------- update all NPCs --------
    for (auto &npc: npcs) {
        if (npc.type == NPC::Type::HUMAN) {
            HumanBehavior::Update(*this, npc, dt);
        }
    }

    // -------- cleanup bandits outside map --------
    npcs.erase(
            std::remove_if(npcs.begin(), npcs.end(),
                           [&](const NPC &n) {
                               return n.humanRole == NPC::HumanRole::BANDIT &&
                                      (n.pos.x < -100 || n.pos.y < -100 ||
                                       n.pos.x > worldW + 100 || n.pos.y > worldH + 100);
                           }),
            npcs.end()
    );
    npcs.erase(
            std::remove_if(npcs.begin(), npcs.end(),
                           [](const NPC &n) { return !n.alive; }),
            npcs.end()
    );
    for (auto &s: settlements) {
        if (!s.alive) continue;

        bool anyoneLeft = false;
        for (const auto &npc: npcs) {
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
    MergeSettlementsIfNeeded();
}

// ------------------------------------------------------------
// Draw
// ------------------------------------------------------------
Color GetSafeSettlementColor(const World &w, int sid) {
    if (sid < 0 || sid >= (int) w.settlements.size() || !w.settlements[sid].alive) {
        return Color{220, 220, 220, 255}; // серый для "без поселения"
    }
    return w.settlements[sid].color;
}

void World::Draw() const {
    // grass
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            Color base = ((x + y) % 2 == 0)
                         ? Color{60, 120, 60, 255}
                         : Color{55, 110, 55, 255};
            DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, base);
        }
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

    // NPCs
    for (const auto &npc: npcs) {
        float size = CELL_SIZE * 0.4f;

        Color c = GetSafeSettlementColor(*this, npc.settlementId);

        switch (npc.humanRole) {
            case NPC::HumanRole::CIVILIAN:
                DrawCircleV(npc.pos, size, c);
                break;
            case NPC::HumanRole::WARRIOR:
                DrawRectangle(npc.pos.x - size, npc.pos.y - size, size * 2, size * 2, c);
                break;
            case NPC::HumanRole::BANDIT:
                DrawBanditTriangle(npc.pos, CELL_SIZE * 0.7f, Color{160, 80, 200, 255});
                break;
            default:
                break;
        }
    }
}
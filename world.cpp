#include "world.h"
#include "human_behavior.h"
#include "bandit_behavior.h"
#include "settlement.h"
#include "npc.h"
#include <algorithm>
#include <cmath>

// ===== Bomb constructor =====
Bomb::Bomb(float x, float y, float fuse_time, float explosion_radius)
    : x(x), y(y), timer(fuse_time), radius(explosion_radius), exploded(false) {}

// ===== ExplosionEffect constructor & update =====
void ExplosionEffect::Update(float dt) {
    life += dt;
}

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
static bool RectsOverlap(const Rectangle& a, const Rectangle& b) {
    return (a.x < b.x + b.width) && (a.x + a.width > b.x) &&
           (a.y < b.y + b.height) && (a.y + a.height > b.y);
}
static Rectangle ZoneToPxRect(const Rectangle& z) {
    return Rectangle{
            z.x * CELL_SIZE,
            z.y * CELL_SIZE,
            z.width * CELL_SIZE,
            z.height * CELL_SIZE
    };
}

// ------------------------------------------------------------
void World::MergeSettlementsIfNeeded() {
    for (int i = 0; i < (int)settlements.size(); i++) {
        if (!settlements[i].alive) continue;

        for (int j = i + 1; j < (int)settlements.size(); j++) {
            if (!settlements[j].alive) continue;

            bool overlap = false;

            for (const auto& ra : settlements[i].zones) {
                Rectangle aPx = {
                        ra.x * CELL_SIZE,
                        ra.y * CELL_SIZE,
                        ra.width * CELL_SIZE,
                        ra.height * CELL_SIZE
                };

                for (const auto& rb : settlements[j].zones) {
                    Rectangle bPx = {
                            rb.x * CELL_SIZE,
                            rb.y * CELL_SIZE,
                            rb.width * CELL_SIZE,
                            rb.height * CELL_SIZE
                    };

                    if (CheckCollisionRecs(aPx, bPx)) {
                        overlap = true;
                        break;
                    }
                }
                if (overlap) break;
            }

            if (!overlap) continue;

            // ---- MERGE j -> i ----
            for (const auto& r : settlements[j].zones) {
                settlements[i].zones.push_back(r);
            }

            settlements[i].centerPx = ComputeSettlementCenterPx(settlements[i]);

            for (auto& npc : npcs) {
                if (npc.settlementId == j)
                    npc.settlementId = i;
            }

            settlements[j].alive = false;
        }
    }
}

static Vector2 RandomEdgeSpawn(int w, int h) {
    int side = GetRandomValue(0, 3);
    switch (side) {
        case 0: return { 0.0f, (float)GetRandomValue(0, h) };        // left
        case 1: return { (float)w, (float)GetRandomValue(0, h) };    // right
        case 2: return { (float)GetRandomValue(0, w), 0.0f };        // top
        default:return { (float)GetRandomValue(0, w), (float)h };   // bottom
    }
}
void World::SpawnCivilian(Vector2 pos) {
    NPC npc;
    npc.type = NPC::Type::HUMAN;
    npc.humanRole = NPC::HumanRole::CIVILIAN;

    npc.pos = pos;
    npc.vel = {0, 0};

    npc.hp = 100.0f;
    npc.damage = 0.0f;

    // 1) если клик внутри существующего поселения — просто присоединяем
    int sid = -1;
    for (int i = 0; i < (int)settlements.size(); i++) {
        if (!settlements[i].alive) continue;
        if (PointInSettlementPx(settlements[i], pos)) { sid = i; break; }
    }

    // 2) иначе — ищем рядом "свободных" жителей без поселения
    if (sid == -1) {
        std::vector<int> nearbyFreeCivs;
        for (int i = 0; i < (int)npcs.size(); i++) {
            const auto& o = npcs[i];
            if (!o.alive) continue;
            if (o.humanRole != NPC::HumanRole::CIVILIAN) continue;
            if (o.settlementId != -1) continue;

            float dx = o.pos.x - pos.x;
            float dy = o.pos.y - pos.y;
            if (dx*dx + dy*dy < 90.0f*90.0f) nearbyFreeCivs.push_back(i);
        }

        // если есть 2 рядом — создаём новое поселение (3-й будет текущий npc)
        if ((int)nearbyFreeCivs.size() >= 2) {
            Settlement s;
            s.alive = true;
            s.color = Color{
                    (unsigned char)GetRandomValue(80,255),
                    (unsigned char)GetRandomValue(80,255),
                    (unsigned char)GetRandomValue(80,255),
                    255
            };

            int cx = (int)(pos.x / CELL_SIZE);
            int cy = (int)(pos.y / CELL_SIZE);
            s.zones.push_back({ (float)(cx-4), (float)(cy-4), 8, 8 });
            s.centerPx = ComputeSettlementCenterPx(s);

            settlements.push_back(s);
            sid = (int)settlements.size() - 1;

            // присваиваем этим двум + новому
            npcs[nearbyFreeCivs[0]].settlementId = sid;
            npcs[nearbyFreeCivs[1]].settlementId = sid;
        }
    }

    npc.settlementId = sid;  // может остаться -1, если это 1-й/2-й житель
    npcs.push_back(npc);
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

    int groupId = -1;
    for (const auto& other : npcs) {
        if (other.humanRole != NPC::HumanRole::WARRIOR) continue;

        float dx = other.pos.x - pos.x;
        float dy = other.pos.y - pos.y;
        if (dx*dx + dy*dy < 80*80) {
            groupId = other.banditGroupId; // временно используем как squadId
            break;
        }
    }

    if (groupId == -1) groupId = nextBanditGroupId++;

    npc.banditGroupId = groupId;
    npcs.push_back(npc);
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
    bombs.clear();
    explosionEffects.clear(); // <-- добавлено

    banditSpawnTimer = 0.0f;
    nextBanditGroupId = 1;
}

// ------------------------------------------------------------
// Bomb implementation
// ------------------------------------------------------------

void Bomb::Update(World& world, float dt) {
    if (exploded) return;
    timer -= dt;
    if (timer <= 0.0f) {
        Explode(world);
    }
}

void Bomb::Explode(World& world) {
    if (exploded) return;
    exploded = true;

    for (auto& npc : world.npcs) {
        if (!npc.alive) continue;
        float dx = npc.pos.x - x;
        float dy = npc.pos.y - y;
        float dist_sq = dx * dx + dy * dy;
        if (dist_sq <= radius * radius) {
            npc.hp -= 50.0f;
            if (npc.hp <= 0) npc.alive = false;

            float dist = std::sqrt(dist_sq);
            if (dist > 0.0f) {
                float force = 300.0f;
                npc.vel.x += (dx / dist) * force;
                npc.vel.y += (dy / dist) * force;
            }
        }
    }

    // Добавляем визуальный эффект взрыва
    world.explosionEffects.push_back({x, y});
}

void World::AddBomb(float x, float y) {
    bombs.emplace_back(x, y, 2.0f, 60.0f);
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

    // -------- update bombs --------
    for (auto& bomb : bombs) {
        bomb.Update(*this, dt);
    }
    bombs.erase(
        std::remove_if(bombs.begin(), bombs.end(),
            [](const Bomb& b) { return b.exploded; }),
        bombs.end()
    );

    // -------- update explosion effects --------
    for (auto& fx : explosionEffects) {
        fx.Update(dt);
    }
    explosionEffects.erase(
        std::remove_if(explosionEffects.begin(), explosionEffects.end(),
            [](const ExplosionEffect& e) { return !e.IsAlive(); }),
        explosionEffects.end()
    );

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
    MergeSettlementsIfNeeded();
}

// ------------------------------------------------------------
// Draw
// ------------------------------------------------------------
Color GetSafeSettlementColor(const World& w, int sid) {
    if (sid < 0 || sid >= (int)w.settlements.size() || !w.settlements[sid].alive) {
        return Color{220, 220, 220, 255}; // серый для "без поселения"
    }
    return w.settlements[sid].color;
}

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

        Color c = GetSafeSettlementColor(*this, npc.settlementId);

        switch (npc.humanRole) {
            case NPC::HumanRole::CIVILIAN:
                DrawCircleV(npc.pos, size, c);
                break;
            case NPC::HumanRole::WARRIOR:
                DrawRectangle(npc.pos.x - size, npc.pos.y - size, size*2, size*2, c);
                break;
            case NPC::HumanRole::BANDIT:
                DrawBanditTriangle(npc.pos, CELL_SIZE*0.7f, Color{160,80,200,255});
                break;
            default:
                break;
        }
    }

    // Bombs
    for (const auto& bomb : bombs) {
        if (!bomb.exploded) {
            DrawCircle((int)bomb.x, (int)bomb.y, 8, BLACK);
            DrawCircleLines((int)bomb.x, (int)bomb.y, 8, RED);
        }
    }

    // Explosion effects
    for (const auto& fx : explosionEffects) {
        float t = fx.life / fx.maxLife; // от 0.0 до 1.0
        float radius = fx.maxRadius * t;
        Color color = Color{
            (unsigned char)(255 * (1.0f - t)),
            (unsigned char)(200 * (1.0f - t * 0.7f)),
            0,
            (unsigned char)(200 * (1.0f - t))
        };
        DrawCircle((int)fx.x, (int)fx.y, (int)radius, color);
    }
}
#include "civilian_behavior.h"
#include "world.h"

void CivilianBehavior::Update(World& world, NPC& npc, float dt) {
    // --- если не привязан к поселению ---
    if (npc.settlementId == -1) {
        // хаотичное блуждание
        npc.wanderTimer -= dt;
        if (npc.wanderTimer <= 0.0f) {
            npc.wanderTimer = RandomFloat(1.5f, 4.0f);
            npc.wanderDir = SafeNormalize(RandomUnit2D());
        }

        npc.vel = {
                npc.wanderDir.x * npc.speed,
                npc.wanderDir.y * npc.speed
        };

        npc.pos.x += npc.vel.x * dt;
        npc.pos.y += npc.vel.y * dt;

        // проверка: вошёл ли в поселение
        for (int i = 0; i < (int)world.settlements.size(); i++) {
            if (!world.settlements[i].alive) continue;
            if (PointInSettlementPx(world.settlements[i], npc.pos)) {
                npc.settlementId = i;
                break;
            }
        }

        return; // важно
    }
    if (npc.settlementId < 0 || npc.settlementId >= (int)world.settlements.size() ||
        !world.settlements[npc.settlementId].alive) {
        // Пока нет поселения — просто свободное перемещение по миру
        Vector2 dir = SafeNormalize(npc.wanderDir);
        npc.vel = Vector2Scale(dir, npc.speed);
        npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));

        // держим в границах карты
        npc.pos.x = Clamp(npc.pos.x, 0.0f, (float)world.worldW);
        npc.pos.y = Clamp(npc.pos.y, 0.0f, (float)world.worldH);

        return;
    }
    const Settlement& s = world.settlements[npc.settlementId];

    // ---------- обновление таймеров ----------
    if (npc.isIdle) {
        npc.idleTimer -= dt;
        if (npc.idleTimer <= 0.0f) {
            npc.isIdle = false;
            npc.moveTimer = RandomFloat(3.5f, 7.0f); // идём 2–5 сек
        }
    } else {
        npc.moveTimer -= dt;
        if (npc.moveTimer <= 0.0f) {
            npc.isIdle = true;
            npc.idleTimer = RandomFloat(0.25f, 0.8f); // пауза 0.5–2 сек
            npc.vel = {0.0f, 0.0f};
        }
    }
// обновляем направление ТОЛЬКО когда начинаем движение
    if (!npc.isIdle && npc.moveTimer > 0.0f && npc.moveTimer < dt + 0.05f) {
        npc.wanderDir = SafeNormalize(RandomUnit2D());
    }
    // ---------- если отдыхаем ----------
    if (npc.isIdle) {
        return;
    }

    // ---------- плавное блуждание ----------


    Vector2 desiredDir;

    bool inside = PointInSettlementPx(s, npc.pos);

    if (!inside) {
        // выбираем домашний тайл ОДИН РАЗ
        if (npc.homeTile == -1 && !s.tiles.empty()) {
            int index = GetRandomValue(0, (int)s.tiles.size() - 1);
            auto it = s.tiles.begin();
            std::advance(it, index);
            npc.homeTile = *it;
        }

        if (npc.homeTile != -1) {
            int cx = npc.homeTile % world.cols;
            int cy = npc.homeTile / world.cols;

            Vector2 target = {
                    (cx + 0.5f) * CELL_SIZE,
                    (cy + 0.5f) * CELL_SIZE
            };

            desiredDir = SafeNormalize(Vector2Subtract(target, npc.pos));
        } else {
            desiredDir = npc.wanderDir;
        }
    } else {
        npc.homeTile = -1;
        desiredDir = npc.wanderDir;
    }

    // ---------- скорость (умеренная) ----------
    float speed = npc.speed * 1.15f;

    // ---------- инерция ----------
    npc.vel = Vector2Scale(desiredDir, speed);
    npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));
    // паника
    bool danger = false;
    for (const auto& other : world.npcs) {
        if (other.humanRole == NPC::HumanRole::BANDIT) {
            float dx = other.pos.x - npc.pos.x;
            float dy = other.pos.y - npc.pos.y;
            if (dx*dx + dy*dy < 220.0f * 220.0f) {
                danger = true;
                break;
            }
        }
    }
    if (danger) {
        // паника: убегаем от ближайшего бандита, а не от центра
        Vector2 away{0,0};
        float minDist = 1e9f;

        for (const auto& other : world.npcs) {
            if (other.humanRole != NPC::HumanRole::BANDIT) continue;

            float dx = npc.pos.x - other.pos.x;
            float dy = npc.pos.y - other.pos.y;
            float d2 = dx*dx + dy*dy;

            if (d2 < minDist) {
                minDist = d2;
                away = { dx, dy };
            }
        }

        if (minDist < 220.0f * 220.0f) {
            desiredDir = SafeNormalize(away);
            speed *= 1.3f;
        }
    }
    npc.vel = Vector2Scale(desiredDir, speed);
    npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));
}
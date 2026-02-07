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
            npc.moveTimer = RandomFloat(2.0f, 5.0f); // идём 2–5 сек
        }
    } else {
        npc.moveTimer -= dt;
        if (npc.moveTimer <= 0.0f) {
            npc.isIdle = true;
            npc.idleTimer = RandomFloat(0.5f, 2.0f); // пауза 0.5–2 сек
            npc.vel = {0.0f, 0.0f};
        }
    }

    // ---------- если отдыхаем ----------
    if (npc.isIdle) {
        return;
    }

    // ---------- плавное блуждание ----------
    Vector2 noise = {
            RandomFloat(-1.0f, 1.0f),
            RandomFloat(-1.0f, 1.0f)
    };
    noise = SafeNormalize(noise);

    npc.wanderDir.x = npc.wanderDir.x * 0.9f + noise.x * 0.1f;
    npc.wanderDir.y = npc.wanderDir.y * 0.9f + noise.y * 0.1f;
    npc.wanderDir = SafeNormalize(npc.wanderDir);

    Vector2 desiredDir;

    bool inside = PointInSettlementPx(s, npc.pos);
    if (inside) {
        desiredDir = npc.wanderDir;
    } else {
        // мягко возвращаемся в поселение
        Vector2 toCenter = {
                s.centerPx.x - npc.pos.x,
                s.centerPx.y - npc.pos.y
        };
        desiredDir = SafeNormalize(toCenter);
    }

    // ---------- скорость (умеренная) ----------
    float speed = npc.speed * 0.95f;

    // ---------- инерция ----------
    npc.vel.x = npc.vel.x * 0.7f + desiredDir.x * speed * 0.3f;
    npc.vel.y = npc.vel.y * 0.7f + desiredDir.y * speed * 0.3f;

    npc.pos.x += npc.vel.x * dt;
    npc.pos.y += npc.vel.y * dt;
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
        Vector2 away = {
                npc.pos.x - s.centerPx.x,
                npc.pos.y - s.centerPx.y
        };
        desiredDir = SafeNormalize(away);
        speed *= 1.3f;
    }
}
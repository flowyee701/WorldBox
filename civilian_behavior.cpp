#include "civilian_behavior.h"
#include "world.h"

void CivilianBehavior::Update(World& world, NPC& npc, float dt) {
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
}
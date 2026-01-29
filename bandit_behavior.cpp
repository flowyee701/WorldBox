#include "bandit_behavior.h"
#include "world.h"
#include "civilian_behavior.h"


void BanditBehavior::Update(World& world, NPC& npc, float dt) {

    // считаем время жизни
    npc.banditLifeTime += dt;

    // проверяем, в поселении ли
    const Settlement* targetSettlement = nullptr;
    for (const auto& s : world.settlements) {
        if (PointInSettlementPx(s, npc.pos)) {
            targetSettlement = &s;
            break;
        }
    }


    // ❌ не нашли поселение за 30 секунд — исчезаем
    if (!targetSettlement && npc.banditLifeTime > 30.0f) {
        // выкидываем за карту → world.cpp сам удалит
        npc.pos = { -1000.0f, -1000.0f };
        return;
    }

    // ---------- движение ----------
    float baseSpeed = npc.speed * 0.55f;
    Vector2 desiredDir;

    if (targetSettlement) {
        Vector2 toCenter = {
                targetSettlement->centerPx.x - npc.pos.x,
                targetSettlement->centerPx.y - npc.pos.y
        };
        desiredDir = SafeNormalize(toCenter);
        baseSpeed *= 0.5f; // медленно при налёте
    } else {
        desiredDir = npc.banditGroupDir;
    }

    // шум / произвольность
    float noiseStrength = 0.6f;
    Vector2 noise = {
            RandomFloat(-1.0f, 1.0f),
            RandomFloat(-1.0f, 1.0f)
    };
    noise = SafeNormalize(noise);

    desiredDir.x = desiredDir.x * (1.0f - noiseStrength) + noise.x * noiseStrength;
    desiredDir.y = desiredDir.y * (1.0f - noiseStrength) + noise.y * noiseStrength;
    desiredDir = SafeNormalize(desiredDir);

    // инерция
    npc.vel.x = npc.vel.x * 0.85f + desiredDir.x * baseSpeed * 0.15f;
    npc.vel.y = npc.vel.y * 0.85f + desiredDir.y * baseSpeed * 0.15f;

    npc.pos.x += npc.vel.x * dt;
    npc.pos.y += npc.vel.y * dt;
    // --- не даём выйти за границы мира ---
    const float margin = 5.0f;

    npc.pos.x = Clamp(npc.pos.x, margin, world.worldW - margin);
    npc.pos.y = Clamp(npc.pos.y, margin, world.worldH - margin);
}
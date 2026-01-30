#include "bandit_behavior.h"
#include "world.h"
#include "civilian_behavior.h"


void BanditBehavior::Update(World& world, NPC& npc, float dt) {

    npc.attackCooldown -= dt;
    if (npc.attackCooldown < 0.0f)
        npc.attackCooldown = 0.0f;
    // считаем время жизни
    npc.banditLifeTime += dt;



    const float AGGRO_RADIUS = 140.0f;

    // проверяем, в поселении ли
    const Settlement* targetSettlement = nullptr;
    NPC* victim = nullptr;
    float bestDist2 = 1e9f;

// ищем ближайшего жителя или воина В ЭТОМ ПОСЕЛЕНИИ
    if (targetSettlement) {
        for (auto& other : world.npcs) {
            if (!other.alive) continue;
            if (other.humanRole == NPC::HumanRole::BANDIT) continue;
            if (other.settlementId != npc.settlementId) continue;

            float dx = other.pos.x - npc.pos.x;
            float dy = other.pos.y - npc.pos.y;
            float d2 = dx*dx + dy*dy;

            if (d2 < bestDist2) {
                bestDist2 = d2;
                victim = &other;
            }
        }
    }
    for (const auto& s : world.settlements) {
        if (PointInSettlementPx(s, npc.pos)) {
            targetSettlement = &s;
            break;
        }
    }


    // ❌ не нашли поселение за 30 секунд — исчезаем
    if (!targetSettlement && npc.banditLifeTime > 90.0f) {
        // выкидываем за карту → world.cpp сам удалит
        npc.pos = { -1000.0f, -1000.0f };
        return;
    }

    // ---------- движение ----------
    float baseSpeed = npc.speed * 0.55f;
    Vector2 desiredDir;
    NPC* targetWarrior = nullptr;

// ищем ближайшего воина
    for (auto& other : world.npcs) {
        if (!other.alive) continue;
        if (other.humanRole != NPC::HumanRole::WARRIOR) continue;

        float dx = other.pos.x - npc.pos.x;
        float dy = other.pos.y - npc.pos.y;
        float d2 = dx*dx + dy*dy;

        if (d2 < AGGRO_RADIUS * AGGRO_RADIUS && d2 < bestDist2) {
            bestDist2 = d2;
            targetWarrior = &other;
        }
    }

    if (targetSettlement) {
        Vector2 toCenter = {
                targetSettlement->centerPx.x - npc.pos.x,
                targetSettlement->centerPx.y - npc.pos.y
        };
        if (victim) {
            // идём к жертве
            Vector2 toVictim = {
                    victim->pos.x - npc.pos.x,
                    victim->pos.y - npc.pos.y
            };
            desiredDir = SafeNormalize(toVictim);
        }
        else if (targetSettlement) {
            // рейд: блуждаем по поселению
            Vector2 noise = {
                    RandomFloat(-1.0f, 1.0f),
                    RandomFloat(-1.0f, 1.0f)
            };
            noise = SafeNormalize(noise);

            npc.banditGroupDir.x = npc.banditGroupDir.x * 0.8f + noise.x * 0.2f;
            npc.banditGroupDir.y = npc.banditGroupDir.y * 0.8f + noise.y * 0.2f;
            npc.banditGroupDir = SafeNormalize(npc.banditGroupDir);

            desiredDir = npc.banditGroupDir;
        }
        else {
            // вне поселения — старая логика
            desiredDir = npc.banditGroupDir;
        }
        baseSpeed *= 0.5f; // медленно при налёте
    } else {
        if (targetWarrior) {
            // вся группа идёт к точке боя
            Vector2 toEnemy = {
                    targetWarrior->pos.x - npc.pos.x,
                    targetWarrior->pos.y - npc.pos.y
            };
            desiredDir = SafeNormalize(toEnemy);

            // обновляем направление группы
            npc.banditGroupDir = desiredDir;
        } else {
            desiredDir = npc.banditGroupDir;
        };
    }

    // шум / произвольность
    float noiseStrength = targetWarrior ? 0.2f : 0.6f;
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
    for (auto& other : world.npcs) {
        if (!other.alive) continue;
        if (other.settlementId == npc.settlementId) continue;
        if (other.humanRole == NPC::HumanRole::BANDIT) continue;

        float dx = other.pos.x - npc.pos.x;
        float dy = other.pos.y - npc.pos.y;

        if (dx*dx + dy*dy < 16.0f * 16.0f) {
            if (npc.attackCooldown <= 0.0f) {
                other.hp -= npc.damage;
                npc.attackCooldown = 1.5f;

                if (other.hp <= 0.0f) {
                    other.alive = false;
                }
            }
            if (other.hp <= 0.0f) {
                other.alive = false;
            }
        }
    }
}
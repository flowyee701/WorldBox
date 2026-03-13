#include "npc/bandit_behavior.h"
#include "environment/world.h"
#include "npc/civilian_behavior.h"

// Updates bandit movement and combat behavior
void BanditBehavior::Update(World& world, NPC& npc, float dt) {
    npc.attackCooldown -= dt;
    if (npc.attackCooldown < 0.0f) npc.attackCooldown = 0.0f;

    npc.banditLifeTime += dt;

    const float AGGRO_RADIUS = 140.0f;

    const Settlement* targetSettlement = nullptr;
    NPC* victim = nullptr;
    float bestDist2 = 1e9f;
    for (const auto& s : world.settlements) {
        if (PointInSettlementPx(s, npc.pos)) {
            targetSettlement = &s;
            break;
        }
    }

    // Despawn bandits that never find a settlement
    if (!targetSettlement && npc.banditLifeTime > 90.0f) {
        npc.pos = { -1000.0f, -1000.0f };
        return;
    }

    float terrainSpeed = world.terrain.getMoveSpeedAt(npc.pos.x, npc.pos.y);
    float swimSpeed = 0.3f;
    float effectiveSpeed = (terrainSpeed > 0.0f) ? terrainSpeed : swimSpeed;
    float baseSpeed = npc.speed * 0.55f * effectiveSpeed;
    Vector2 desiredDir;
    NPC* targetWarrior = nullptr;

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
            Vector2 noise = {
                    RandomFloat(-1.0f, 1.0f),
                    RandomFloat(-1.0f, 1.0f)
            };
            noise = SafeNormalize(noise);

            npc.banditGroupDir.x = npc.banditGroupDir.x * 0.8f + noise.x * 0.2f;
            npc.banditGroupDir.y = npc.banditGroupDir.y * 0.8f + noise.y * 0.2f;
            npc.banditGroupDir = SafeNormalize(npc.banditGroupDir);

            desiredDir = npc.banditGroupDir;
        baseSpeed *= 0.5f;
    } else {
        if (targetWarrior) {
            Vector2 toEnemy = {
                    targetWarrior->pos.x - npc.pos.x,
                    targetWarrior->pos.y - npc.pos.y
            };
            desiredDir = SafeNormalize(toEnemy);

            npc.banditGroupDir = desiredDir;
        } else {
            desiredDir = npc.banditGroupDir;
        };
    }

    float noiseStrength = targetWarrior ? 0.2f : 0.6f;
    Vector2 noise = {
            RandomFloat(-1.0f, 1.0f),
            RandomFloat(-1.0f, 1.0f)
    };
    noise = SafeNormalize(noise);

    desiredDir.x = desiredDir.x * (1.0f - noiseStrength) + noise.x * noiseStrength;
    desiredDir.y = desiredDir.y * (1.0f - noiseStrength) + noise.y * noiseStrength;
    desiredDir = SafeNormalize(desiredDir);

    npc.vel.x = npc.vel.x * 0.85f + desiredDir.x * baseSpeed * 0.15f;
    npc.vel.y = npc.vel.y * 0.85f + desiredDir.y * baseSpeed * 0.15f;

    npc.pos.x += npc.vel.x * dt;
    npc.pos.y += npc.vel.y * dt;

    // Keep bandits inside the world bounds
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
                world.BeginNpcAttack(npc, other.pos);
                other.hp -= npc.damage;
                npc.attackCooldown = 1.00f;

                if (other.hp <= 0.0f) {
                    other.hp = 0.0f;
                    world.BeginNpcDeath(other);
                }
            }
        }
    }
}

#include "warrior_behavior.h"
#include "world.h"
#include "civilian_behavior.h"

void WarriorBehavior::Update(World& world, NPC& npc, float dt) {
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
    if (!npc.formationAssigned) {
        // случайная позиция в радиусе отряда
        float angle = RandomFloat(0.0f, 2.0f * PI);
        float radius = RandomFloat(20.0f, 50.0f);

        npc.formationOffset = {
                cosf(angle) * radius,
                sinf(angle) * radius
        };

        npc.formationAssigned = true;
    }
    npc.attackCooldown -= dt;
    if (npc.attackCooldown < 0.0f)
        npc.attackCooldown = 0.0f;
    const Settlement& s = world.settlements[npc.settlementId];
    const float ATTACK_RADIUS = 170.0f;     // было ~260: реагируют ближе
    const float ALERT_RADIUS  = 260.0f;     // радиус тревоги для всего отряда (см. пункт 3)
    const float MAX_CHASE_RADIUS = 260.0f;  // не уходить далеко от поселения
    NPC* alertBandit = nullptr;
    float bestToCenter2 = 1e9f;

// 1) ищем бандита, который ближе всего к центру поселения
    for (auto& other : world.npcs) {
        if (other.humanRole != NPC::HumanRole::BANDIT) continue;

        float dx = other.pos.x - s.centerPx.x;
        float dy = other.pos.y - s.centerPx.y;
        float d2 = dx*dx + dy*dy;

        if (d2 < bestToCenter2) {
            bestToCenter2 = d2;
            alertBandit = &other;
        }
    }

    bool squadAlert = (alertBandit && bestToCenter2 < ALERT_RADIUS * ALERT_RADIUS);

    // -----------------------------
    // 1. Поиск ближайшего бандита
    // -----------------------------
    NPC* closestBandit = nullptr;
    float closestDist2 = 1e9f;

    for (auto& other : world.npcs) {
        if (other.humanRole != NPC::HumanRole::BANDIT) continue;

        float dx = other.pos.x - npc.pos.x;
        float dy = other.pos.y - npc.pos.y;
        float d2 = dx*dx + dy*dy;

        if (d2 < closestDist2) {
            closestDist2 = d2;
            closestBandit = &other;
        }
    }


    bool banditNearby = closestBandit && closestDist2 < ATTACK_RADIUS * ATTACK_RADIUS;

    // расстояние до центра поселения
    float dxHome = s.centerPx.x - npc.pos.x;
    float dyHome = s.centerPx.y - npc.pos.y;
    float distHome2 = dxHome*dxHome + dyHome*dyHome;

    Vector2 desiredDir;

    // -----------------------------
    // 2. Выбор режима
    // -----------------------------
    if (squadAlert && distHome2 < MAX_CHASE_RADIUS * MAX_CHASE_RADIUS) {
        // ⚔ АТАКА
        Vector2 targetPos = {
                alertBandit->pos.x + npc.formationOffset.x,
                alertBandit->pos.y + npc.formationOffset.y
        };

        Vector2 toTarget = {
                targetPos.x - npc.pos.x,
                targetPos.y - npc.pos.y
        };

        desiredDir = SafeNormalize(toTarget);
    }
    else {
        // проверим: есть ли вообще бандиты рядом
        bool anyBanditsNear = false;
        for (auto& other : world.npcs) {
            if (other.humanRole != NPC::HumanRole::BANDIT) continue;

            float dx = other.pos.x - npc.pos.x;
            float dy = other.pos.y - npc.pos.y;
            if (dx*dx + dy*dy < ATTACK_RADIUS * ATTACK_RADIUS) {
                anyBanditsNear = true;
                break;
            }
        }

        if (!anyBanditsNear) {
            // 🔙 ВОЗВРАТ В ПОСЕЛЕНИЕ
            Vector2 homeTarget = {
                    s.centerPx.x + npc.formationOffset.x,
                    s.centerPx.y + npc.formationOffset.y
            };

            Vector2 toHome = {
                    homeTarget.x - npc.pos.x,
                    homeTarget.y - npc.pos.y
            };
        } else {
            // 🛡 ПАТРУЛЬ
            Vector2 noise = {
                    RandomFloat(-1.0f, 1.0f),
                    RandomFloat(-1.0f, 1.0f)
            };
            noise = SafeNormalize(noise);

            npc.wanderDir.x = npc.wanderDir.x * 0.9f + noise.x * 0.1f;
            npc.wanderDir.y = npc.wanderDir.y * 0.9f + noise.y * 0.1f;
            npc.wanderDir = SafeNormalize(npc.wanderDir);

            desiredDir = npc.wanderDir;
        }
    }

    // -----------------------------
    // 3. Cohesion (держимся группой)
    // -----------------------------
    Vector2 cohesion = {0,0};
    int count = 0;

    for (const auto& other : world.npcs) {
        if (other.humanRole != NPC::HumanRole::WARRIOR) continue;
        if (other.settlementId != npc.settlementId) continue;

        float dx = other.pos.x - npc.pos.x;
        float dy = other.pos.y - npc.pos.y;
        float d2 = dx*dx + dy*dy;

        if (d2 > 0.01f && d2 < 90*90) {
            cohesion.x += dx;
            cohesion.y += dy;
            count++;
        }
    }

    if (count > 0) {
        cohesion.x /= count;
        cohesion.y /= count;
        cohesion = SafeNormalize(cohesion);

        desiredDir.x = desiredDir.x * 0.65f + cohesion.x * 0.35f;
        desiredDir.y = desiredDir.y * 0.65f + cohesion.y * 0.35f;
        desiredDir = SafeNormalize(desiredDir);
    }

    // -----------------------------
    // 4. Движение
    // -----------------------------
    float speed = npc.speed * 1.1f; // быстрее, чем жители

    npc.vel.x = npc.vel.x * 0.65f + desiredDir.x * speed * 0.35f;
    npc.vel.y = npc.vel.y * 0.65f + desiredDir.y * speed * 0.35f;

    npc.pos.x += npc.vel.x * dt;
    npc.pos.y += npc.vel.y * dt;
    // атака
    for (auto& other : world.npcs) {
        if (!other.alive) continue;
        if (other.humanRole != NPC::HumanRole::BANDIT) continue;

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
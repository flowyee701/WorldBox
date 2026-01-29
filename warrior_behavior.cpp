#include "warrior_behavior.h"
#include "world.h"
#include "civilian_behavior.h"

void WarriorBehavior::Update(World& world, NPC& npc, float dt) {
    const Settlement& s = world.settlements[npc.settlementId];

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

    const float ATTACK_RADIUS = 260.0f;
    const float MAX_CHASE_RADIUS = 320.0f;

    bool banditNearby = closestBandit && closestDist2 < ATTACK_RADIUS * ATTACK_RADIUS;

    // расстояние до центра поселения
    float dxHome = s.centerPx.x - npc.pos.x;
    float dyHome = s.centerPx.y - npc.pos.y;
    float distHome2 = dxHome*dxHome + dyHome*dyHome;

    Vector2 desiredDir;

    // -----------------------------
    // 2. Выбор режима
    // -----------------------------
    if (banditNearby && distHome2 < MAX_CHASE_RADIUS * MAX_CHASE_RADIUS) {
        // ⚔ АТАКА
        Vector2 toEnemy = {
                closestBandit->pos.x - npc.pos.x,
                closestBandit->pos.y - npc.pos.y
        };
        desiredDir = SafeNormalize(toEnemy);
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
            Vector2 toHome = { dxHome, dyHome };
            desiredDir = SafeNormalize(toHome);
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
}
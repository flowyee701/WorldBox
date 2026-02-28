#include "../include/warrior_behavior.h"
#include "../include/world.h"
#include "../include/civilian_behavior.h"

void WarriorBehavior::Update(World& world, NPC& npc, float dt) {
    // ---------- привязка к поселению ----------
    if (npc.settlementId == -1) {
        for (int i = 0; i < (int)world.settlements.size(); i++) {
            if (!world.settlements[i].alive) continue;

            if (PointInSettlementPx(world.settlements[i], npc.pos)) {
                npc.settlementId = i;
                npc.homeTile = -1; // сброс цели
                npc.formationAssigned = false;
                break;
            }
        }
    }

    if (npc.settlementId < 0 ||
        npc.settlementId >= (int)world.settlements.size() ||
        !world.settlements[npc.settlementId].alive) {
        npc.settlementId = -1;
    }


    if (npc.settlementId == -1) {
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

        return;
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
    NPC* spottedBandit = nullptr;

    for (auto& other : world.npcs) {
        if (!other.alive) continue;
        if (other.humanRole != NPC::HumanRole::BANDIT) continue;

        float dx = other.pos.x - npc.pos.x;
        float dy = other.pos.y - npc.pos.y;
        if (dx*dx + dy*dy < ALERT_RADIUS * ALERT_RADIUS) {
            spottedBandit = &other;
            break;
        }
    }
    if (spottedBandit) {
        for (auto& w : world.npcs) {
            if (!w.alive) continue;
            if (w.humanRole != NPC::HumanRole::WARRIOR) continue;
            if (w.settlementId != npc.settlementId) continue;

            w.combatTargetPos = spottedBandit->pos;
            w.inCombat = true;

            // назначаем оффсет формации (если ещё не был)
            if (!w.formationAssigned) {
                float angle = RandomFloat(0.0f, 2.0f * PI);
                float radius = RandomFloat(20.0f, 60.0f);
                w.formationOffset = {
                        cosf(angle) * radius,
                        sinf(angle) * radius
                };
                w.formationAssigned = true;
            }
        }
    }
    if (npc.inCombat) {
        Vector2 target = {
                npc.combatTargetPos.x + npc.formationOffset.x,
                npc.combatTargetPos.y + npc.formationOffset.y
        };

        Vector2 toTarget = {
                target.x - npc.pos.x,
                target.y - npc.pos.y
        };

        Vector2 dir = SafeNormalize(toTarget);
        npc.vel = {
                dir.x * npc.speed,
                dir.y * npc.speed
        };

        npc.pos.x += npc.vel.x * dt;
        npc.pos.y += npc.vel.y * dt;
        return;
    }

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
    if (!npc.inCombat) {
        npc.formationAssigned = false;
    }

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
            // 🔙 ВОЗВРАТ В ПОСЕЛЕНИЕ (к своему тайлу)
            if (npc.homeTile == -1 && !s.tiles.empty()) {
                int index = GetRandomValue(0, (int)s.tiles.size() - 1);
                auto it = s.tiles.begin();
                std::advance(it, index);
                npc.homeTile = *it;
            }

            if (npc.homeTile != -1) {
                int cx = npc.homeTile % world.cols;
                int cy = npc.homeTile / world.cols;

                Vector2 homeTarget = {
                        (cx + 0.5f) * CELL_SIZE + npc.formationOffset.x,
                        (cy + 0.5f) * CELL_SIZE + npc.formationOffset.y
                };

                desiredDir = SafeNormalize(Vector2Subtract(homeTarget, npc.pos));
            }
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
#include "npc/warrior_behavior.h"
#include "environment/world.h"
#include <cfloat>

static inline float Dist2(Vector2 a, Vector2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx*dx + dy*dy;
}

void WarriorBehavior::Update(World& world, NPC& npc, float dt) {
    // ---- базовые проверки ----
    if (npc.settlementId < 0 || npc.settlementId >= (int)world.settlements.size() ||
        !world.settlements[npc.settlementId].alive) {
        // если поселение потеряно — просто блуждаем как раньше
        npc.wanderTimer -= dt;
        if (npc.wanderTimer <= 0.0f) {
            npc.wanderTimer = RandomFloat(1.0f, 2.5f);
            npc.wanderDir = SafeNormalize(RandomUnit2D());
        }
        npc.vel = Vector2Scale(npc.wanderDir, npc.speed);
        npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));
        return;
    }

    const Settlement& s = world.settlements[npc.settlementId];

    // =========================================================
    // 1) ПОИСК БЛИЖАЙШЕГО БАНДИТА
    // =========================================================
    const float ALERT_R  = 260.0f;     // радиус, в котором начинаем бой
    const float GIVEUP_R = 340.0f;     // радиус, если никого нет -> выходим из боя
    const float ALERT_R2  = ALERT_R * ALERT_R;
    const float GIVEUP_R2 = GIVEUP_R * GIVEUP_R;

    int nearestBandit = -1;
    float nearestD2 = FLT_MAX;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const auto& other = world.npcs[i];
        if (!other.alive) continue;
        if (other.humanRole != NPC::HumanRole::BANDIT) continue;

        float d2 = Dist2(other.pos, npc.pos);
        if (d2 < nearestD2) {
            nearestD2 = d2;
            nearestBandit = i;
        }
    }

    const bool banditInAlert = (nearestBandit != -1 && nearestD2 <= ALERT_R2);
    const bool banditInGiveup = (nearestBandit != -1 && nearestD2 <= GIVEUP_R2);

    // =========================================================
    // 2) ПЕРЕКЛЮЧЕНИЕ СОСТОЯНИЙ БОЙ <-> МИР
    // =========================================================
    if (banditInAlert) {
        npc.inCombat = true;
        npc.combatTargetPos = world.npcs[nearestBandit].pos;
    } else {
        // если рядом никого нет — выходим из боя (это главное исправление!)
        if (npc.inCombat && !banditInGiveup) {
            npc.inCombat = false;
            npc.combatTargetPos = {0,0};
            npc.attackCooldown = 0.0f; // чтобы сразу не залипать на старом
        }
    }

    // =========================================================
    // 3) ЛОГИКА БОЯ
    // =========================================================
    if (npc.inCombat) {
        // обновляем цель, если бандит ещё рядом
        if (nearestBandit != -1 && banditInGiveup) {
            npc.combatTargetPos = world.npcs[nearestBandit].pos;
        }

        Vector2 dir = SafeNormalize(Vector2Subtract(npc.combatTargetPos, npc.pos));
        float speed = npc.speed * 1.6f;

        npc.vel = Vector2Scale(dir, speed);
        npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));

        // атака
        npc.attackCooldown -= dt;
        if (npc.attackCooldown <= 0.0f && nearestBandit != -1 && nearestD2 < (28.0f * 28.0f)) {
            world.npcs[nearestBandit].hp -= npc.damage;
            npc.attackCooldown = 0.65f;

            if (world.npcs[nearestBandit].hp <= 0.0f) {
                world.npcs[nearestBandit].alive = false;
            }
        }

        return;
    }

    // =========================================================
    // 4) "ДОМАШНЕЕ" ПОВЕДЕНИЕ: ТУСОВКА У КОСТРА (ПОКА centerPx)
    // =========================================================
    // будущий костёр: s.campfirePx (пока используем s.centerPx)
    Vector2 camp = s.centerPx;

    // один раз задаём “место в строю” вокруг костра
    if (!npc.formationAssigned) {
        // назначаем смещение формации один раз
        float angle = RandomFloat(0.0f, 2.0f * PI);
        float radius = RandomFloat(20.0f, 50.0f);

        npc.formationOffset = { cosf(angle) * radius, sinf(angle) * radius };
        npc.formationAssigned = true;
    }

    Vector2 target = Vector2Add(camp, npc.formationOffset);

    // если пришёл к точке — начинай слегка бродить вокруг (чтобы не стоял колом)
    float toT2 = Dist2(npc.pos, target);
    if (toT2 < 10.0f * 10.0f) {
        npc.wanderTimer -= dt;
        if (npc.wanderTimer <= 0.0f) {
            npc.wanderTimer = RandomFloat(0.6f, 1.4f);
            npc.wanderDir = SafeNormalize(RandomUnit2D());
        }
        npc.vel = Vector2Scale(npc.wanderDir, npc.speed * 0.6f);
    } else {
        Vector2 dir = SafeNormalize(Vector2Subtract(target, npc.pos));
        npc.vel = Vector2Scale(dir, npc.speed * 1.0f);
    }

    npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));
}
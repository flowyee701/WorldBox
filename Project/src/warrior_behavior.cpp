#include "npc/warrior_behavior.h"
#include "environment/world.h"
#include <cfloat>
#include <vector>
#include <cmath>

static Vector2 Rotate(Vector2 v, float ang) {
    float c = cosf(ang), s = sinf(ang);
    return Vector2{ v.x * c - v.y * s, v.x * s + v.y * c };
}

// MANUAL -> square block, AUTO -> wedge
static Vector2 GetFormationOffset15(int slot, bool autoMode) {
    if (slot < 0) slot = 0;
    if (slot > 14) slot = 14;

    if (!autoMode) {
        // Square-ish: 4x4 minus one (15 slots)
        // базовая ось "вперёд" = +Y
        const float step = 18.0f;
        int idx = slot; // 0..14
        int row = idx / 4;       // 0..3
        int col = idx % 4;       // 0..3
        float x = (col - 1.5f) * step;
        float y = (row - 1.0f) * step; // центр ближе к капитану
        return Vector2{ x, y };
    } else {
        // Wedge: 1 + 2 + 3 + 4 + 5 = 15
        const float dx = 16.0f;
        const float dy = 18.0f;

        int remain = slot;
        int row = 0;
        int rowCount = 1;
        while (remain >= rowCount && row < 5) {
            remain -= rowCount;
            row++;
            rowCount++;
        }
        int col = remain;                 // 0..rowCount-1
        float x = (col - (rowCount - 1) * 0.5f) * dx;
        float y = (row + 1) * dy;
        return Vector2{ x, y };
    }
}

static inline float Dist2(Vector2 a, Vector2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx*dx + dy*dy;
}
static int FindNearestBanditInGroup(const World& world, Vector2 from, int groupId, float maxRangePx) {
    const float r2 = maxRangePx * maxRangePx;
    int best = -1;
    float bestD2 = FLT_MAX;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const auto& o = world.npcs[i];
        if (!o.alive) continue;
        if (o.humanRole != NPC::HumanRole::BANDIT) continue;
        if (o.banditGroupId != groupId) continue;

        float d2 = Dist2(o.pos, from);
        if (d2 <= r2 && d2 < bestD2) {
            bestD2 = d2;
            best = i;
        }
    }
    return best;
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
    // PRIORITY: follow captain formation if assigned
    // =========================================================
    if (npc.humanRole == NPC::HumanRole::WARRIOR && npc.leaderCaptainId != 0) {
        NPC* cap = world.FindNpcById(npc.leaderCaptainId);
        if (cap && cap->alive && cap->humanRole == NPC::HumanRole::CAPTAIN) {

            // 1) Если у капитана есть ATTACK ORDER — бьём группу
            if (cap->captainHasAttackOrder && cap->captainAttackGroupId != -1) {
                int bi = FindNearestBanditInGroup(world, npc.pos, cap->captainAttackGroupId, 99999.0f);
                if (bi != -1) {
                    NPC& b = world.npcs[bi];

                    // движение к цели
                    Vector2 dir = SafeNormalize(Vector2Subtract(b.pos, npc.pos));
                    npc.vel = Vector2Scale(dir, npc.speed * 1.55f);
                    npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));

                    // атака
                    npc.attackCooldown -= dt;
                    const float meleeRange = 28.0f;
                    if (npc.attackCooldown <= 0.0f && Dist2(npc.pos, b.pos) <= meleeRange * meleeRange) {
                        b.hp -= npc.damage;
                        npc.attackCooldown = 0.80f;
                        if (b.hp <= 0.0f) {
                            b.hp = 0.0f;
                            b.alive = false;
                        }
                    }
                    return;
                }
                // если группа уже умерла — просто держим формацию ниже
            }

            // 2) Иначе держим формацию у капитана (приоритет капитана > костра)
            Vector2 heading = {0, 1};
            if (Vector2Length(cap->vel) > 0.01f) heading = SafeNormalize(cap->vel);

            Vector2 slotOffset = GetFormationOffset15(npc.formationSlot, cap->captainAutoMode);

            // поворот слота под heading
            float baseAng = atan2f(1.0f, 0.0f); // ось "вперёд" = +Y
            float ang = atan2f(heading.y, heading.x) - baseAng;
            slotOffset = Rotate(slotOffset, ang);

            Vector2 target = Vector2Add(cap->pos, slotOffset);

            Vector2 dir = SafeNormalize(Vector2Subtract(target, npc.pos));
            npc.vel = Vector2Scale(dir, npc.speed * 1.15f);
            npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));
            return;
        }
    }
    // =========================================================
    // CAPTAIN: player manual move overrides everything
    // =========================================================
    if (npc.humanRole == NPC::HumanRole::CAPTAIN) {
        if (!npc.captainAutoMode && npc.captainHasMoveOrder) {
            Vector2 dir = SafeNormalize(Vector2Subtract(npc.captainMoveTarget, npc.pos));
            npc.vel = Vector2Scale(dir, npc.speed * 1.4f);
            npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));

            // дошли — завершаем приказ
            float d2 = Dist2(npc.pos, npc.captainMoveTarget);
            if (d2 < 10.0f * 10.0f) {
                npc.captainHasMoveOrder = false;
            }
            return; // ВАЖНО: игнорируем угрозы/боевку, капитан слушается только игрока
        }
    }

    // =========================================================
    // FOLLOW CAPTAIN (if assigned to squad) - only when NOT in combat
    // =========================================================
    if (npc.leaderCaptainId != 0) {
        NPC* cap = world.FindNpcById(npc.leaderCaptainId);
        if (cap && cap->humanRole == NPC::HumanRole::CAPTAIN && cap->alive) {

            // если капитан имеет цель (ручную или авто) — держим строй
            // (даже если цели нет, можно держаться рядом)
            const float dx = 22.0f;
            const float dy = 18.0f;

            // slot -> offset (3x5) локально +Y вперёд, без поворота (упрощённо)
            // (поворот можно добавить позже, когда заведём “heading” капитана)
            Vector2 slotOffset{0, 0};
            if (npc.formationSlot >= 0 && npc.formationSlot < 15) {
                int row = npc.formationSlot / 5;
                int col = npc.formationSlot % 5;
                slotOffset = Vector2{ (col - 2) * dx, 20.0f + row * dy };
            }

            Vector2 target = Vector2Add(cap->pos, slotOffset);

            // не ломаем боёвку: если рядом бандит — он и так включит combat ниже,
            // но пока мы тут дадим "строевое" движение.
            Vector2 dir = SafeNormalize(Vector2Subtract(target, npc.pos));
            npc.vel = Vector2Scale(dir, npc.speed * 1.2f);
            npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));

            // дальше пойдёт поиск бандитов и бой — это ок, он может перебить строй
            // если хочешь строгий строй в бою — сделаем отдельным шагом.
        }
    }

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
            npc.attackCooldown = 0.80f;

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
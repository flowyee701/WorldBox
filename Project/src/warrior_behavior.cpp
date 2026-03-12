#include "npc/warrior_behavior.h"
#include "npc/captain_behavior.h"
#include "environment/world.h"
#include <cfloat>

static inline float Dist2(Vector2 a, Vector2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx*dx + dy*dy;
}

static inline Vector2 SafeNormalizeEx(Vector2 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y);
    if (len < 0.0001f) return {0.0f, 1.0f};
    return { v.x / len, v.y / len };
}

static Vector2 RotateFromBaseY(Vector2 local, Vector2 forward) {
    forward = SafeNormalizeEx(forward);
    float baseAng = atan2f(1.0f, 0.0f); // +Y
    float ang = atan2f(forward.y, forward.x) - baseAng;

    float c = cosf(ang);
    float s = sinf(ang);
    return {
            local.x * c - local.y * s,
            local.x * s + local.y * c
    };
}

static void MoveTowards(NPC& npc, Vector2 target, float dt, float speedMul = 1.0f) {
    Vector2 to = Vector2Subtract(target, npc.pos);
    float len = Vector2Length(to);

    if (len < 0.001f) {
        npc.vel = {0, 0};
        return;
    }

    Vector2 dir = SafeNormalizeEx(to);
    npc.vel = Vector2Scale(dir, npc.speed * speedMul);
    npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));
}

static int FindNearestBanditAny(const World& world, Vector2 from, float maxRangePx) {
    const float maxR2 = maxRangePx * maxRangePx;
    int best = -1;
    float bestD2 = FLT_MAX;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const auto& o = world.npcs[i];
        if (!o.alive) continue;
        if (o.humanRole != NPC::HumanRole::BANDIT) continue;

        float d2 = Dist2(o.pos, from);
        if (d2 <= maxR2 && d2 < bestD2) {
            bestD2 = d2;
            best = i;
        }
    }

    return best;
}

static int FindNearestBanditInGroup(const World& world, Vector2 from, int groupId) {
    int best = -1;
    float bestD2 = FLT_MAX;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const auto& o = world.npcs[i];
        if (!o.alive) continue;
        if (o.humanRole != NPC::HumanRole::BANDIT) continue;
        if (o.banditGroupId != groupId) continue;

        float d2 = Dist2(o.pos, from);
        if (d2 < bestD2) {
            bestD2 = d2;
            best = i;
        }
    }

    return best;
}

static int FindNearestEnemyNpcForSettlementWar(World& world, const NPC& attacker, int enemySettlementId, float maxDistPx) {
    float bestD2 = maxDistPx * maxDistPx;
    int bestIndex = -1;
    int bestPriority = 999;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const NPC& other = world.npcs[i];
        if (!other.alive || other.isDying) continue;
        if (other.settlementId != enemySettlementId) continue;
        if (other.id == attacker.id) continue;

        int priority = 999;
        if (other.humanRole == NPC::HumanRole::WARRIOR) priority = 0;
        else if (other.humanRole == NPC::HumanRole::CAPTAIN) priority = 1;
        else if (other.humanRole == NPC::HumanRole::CIVILIAN) priority = 2;
        else continue;

        float dx = other.pos.x - attacker.pos.x;
        float dy = other.pos.y - attacker.pos.y;
        float d2 = dx*dx + dy*dy;

        if (d2 > bestD2) continue;

        if (priority < bestPriority || (priority == bestPriority && d2 < bestD2)) {
            bestPriority = priority;
            bestD2 = d2;
            bestIndex = i;
        }
    }

    return bestIndex;
}

static int FindNearestEnemyCombatNearNpc(World& world, const NPC& npc, float radiusPx) {
    float bestD2 = radiusPx * radiusPx;
    int bestIndex = -1;
    int bestPriority = 999;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const NPC& other = world.npcs[i];
        if (!other.alive || other.isDying) continue;
        if (other.settlementId == npc.settlementId) continue;

        int priority = 999;
        if (other.humanRole == NPC::HumanRole::WARRIOR) priority = 0;
        else if (other.humanRole == NPC::HumanRole::CAPTAIN) priority = 1;
        else continue;

        float dx = other.pos.x - npc.pos.x;
        float dy = other.pos.y - npc.pos.y;
        float d2 = dx*dx + dy*dy;
        if (d2 > bestD2) continue;

        if (priority < bestPriority || (priority == bestPriority && d2 < bestD2)) {
            bestPriority = priority;
            bestD2 = d2;
            bestIndex = i;
        }
    }

    return bestIndex;
}

static int FindNearestAliveBarracksIndex(const Settlement& s, Vector2 fromPos)
{
    float bestD2 = 1e30f;
    int bestIndex = -1;

    for (int i = 0; i < (int)s.barracksList.size(); i++) {
        const Barracks& b = s.barracksList[i];
        if (!b.alive) continue;
        if (b.hp <= 0.0f) continue;

        float dx = b.posPx.x - fromPos.x;
        float dy = b.posPx.y - fromPos.y;
        float d2 = dx * dx + dy * dy;

        if (d2 < bestD2) {
            bestD2 = d2;
            bestIndex = i;
        }
    }

    return bestIndex;
}

static int FindNearestHostileTroopNearSettlement(World& world, int homeSettlementId, Vector2 homePos, float maxDistPx) {
    float bestD2 = maxDistPx * maxDistPx;
    int bestIndex = -1;
    int bestPriority = 999;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const NPC& other = world.npcs[i];
        if (!other.alive || other.isDying) continue;
        if (other.settlementId == homeSettlementId) continue;

        int priority = 999;
        if (other.humanRole == NPC::HumanRole::WARRIOR) priority = 0;
        else if (other.humanRole == NPC::HumanRole::CAPTAIN) priority = 1;
        else if (other.humanRole == NPC::HumanRole::CIVILIAN) priority = 2;
        else continue;

        float dx = other.pos.x - homePos.x;
        float dy = other.pos.y - homePos.y;
        float d2 = dx*dx + dy*dy;
        if (d2 > bestD2) continue;

        if (priority < bestPriority || (priority == bestPriority && d2 < bestD2)) {
            bestPriority = priority;
            bestD2 = d2;
            bestIndex = i;
        }
    }

    return bestIndex;
}

static void TryMeleeAttack(World& world, NPC& attacker, NPC& target, float dt, float cooldownSeconds) {
    attacker.attackCooldown -= dt;
    if (attacker.attackCooldown > 0.0f) return;

    world.BeginNpcAttack(attacker, target.pos);
    target.hp -= attacker.damage;
    attacker.attackCooldown = cooldownSeconds;

    if (target.hp <= 0.0f) {
        target.hp = 0.0f;
        world.BeginNpcDeath(target);
    }
}

// Updates warrior behavior
void WarriorBehavior::Update(World& world, NPC& npc, float dt) {
    if (npc.warAssigned &&
        npc.warTargetSettlementId >= 0 &&
        world.IsSettlementAliveAndValid(npc.warTargetSettlementId))
    {
        // Defensive mode uses own settlement center as anchor
        if (npc.warIsDefender &&
            npc.settlementId >= 0 &&
            npc.settlementId < (int)world.settlements.size())
        {
            npc.warTargetPos = world.settlements[npc.settlementId].centerPx;
        }
        else {
            npc.warTargetPos = world.settlements[npc.warTargetSettlementId].centerPx;
        }

        // If enemy combat units are nearby, break formation and fight freely
        if (npc.warInBattle) {
            int localEnemyIndex = FindNearestEnemyCombatNearNpc(world, npc, CELL_SIZE * 9.0f);
            if (localEnemyIndex != -1) {
                NPC& enemy = world.npcs[localEnemyIndex];

                float dx = enemy.pos.x - npc.pos.x;
                float dy = enemy.pos.y - npc.pos.y;
                float d2 = dx*dx + dy*dy;

                if (d2 <= 18.0f * 18.0f) {
                    TryMeleeAttack(world, npc, enemy, dt, 0.9f);
                } else {
                    MoveTowards(npc, enemy.pos, dt);
                }
                return;
            }

            Vector2 searchPos = {
                npc.pos.x + (float)((npc.id % 3) - 1) * CELL_SIZE * 0.8f,
                npc.pos.y + (float)(((npc.id / 3) % 3) - 1) * CELL_SIZE * 0.8f
            };
            MoveTowards(npc, searchPos, dt, 0.8f);
            return;
        }

        // Keep cohesion only when not in active battle
        if (npc.warCaptainId != 0) {
            NPC* captain = world.FindNpcById(npc.warCaptainId);
            if (captain && captain->alive && !captain->isDying) {
                float dxCap = captain->pos.x - npc.pos.x;
                float dyCap = captain->pos.y - npc.pos.y;
                float d2Cap = dxCap*dxCap + dyCap*dyCap;

                if (d2Cap > CELL_SIZE * CELL_SIZE * 5.0f) {
                    MoveTowards(npc, captain->pos, dt);
                    return;
                }
            }
        }

        // Normal war target acquisition while approaching
        int enemyIndex = FindNearestEnemyNpcForSettlementWar(world, npc, npc.warTargetSettlementId, CELL_SIZE * 26.0f);
        if (enemyIndex != -1) {
            NPC& enemy = world.npcs[enemyIndex];

            float dx = enemy.pos.x - npc.pos.x;
            float dy = enemy.pos.y - npc.pos.y;
            float d2 = dx*dx + dy*dy;

            if (d2 <= 18.0f * 18.0f) {
                TryMeleeAttack(world, npc, enemy, dt, 0.9f);
            } else {
                MoveTowards(npc, enemy.pos, dt);
            }

            return;
        }

        // If enemy troops are gone, attack the barracks
        if (!world.SettlementHasLivingCombatUnits(npc.warTargetSettlementId)) {
            const Settlement& targetSettlement = world.settlements[npc.warTargetSettlementId];
            int barracksIndex = FindNearestAliveBarracksIndex(targetSettlement, npc.pos);
            if (barracksIndex != -1) {
                const Barracks& targetBarracks = targetSettlement.barracksList[barracksIndex];

                float dx = targetBarracks.posPx.x - npc.pos.x;
                float dy = targetBarracks.posPx.y - npc.pos.y;
                float d2 = dx*dx + dy*dy;

                if (d2 <= 22.0f * 22.0f) {
                    npc.attackCooldown -= dt;
                    if (npc.attackCooldown <= 0.0f) {
                        world.BeginNpcAttack(npc, targetBarracks.posPx);
                        world.DamageSettlementBarracks(npc.warTargetSettlementId, barracksIndex, npc.damage);
                        npc.attackCooldown = 0.9f;
                    }
                } else {
                    MoveTowards(npc, targetBarracks.posPx, dt);
                }
                return;
            }
        }

        // Continue advancing toward the war target
        float dx = npc.warTargetPos.x - npc.pos.x;
        float dy = npc.warTargetPos.y - npc.pos.y;
        float dist2 = dx*dx + dy*dy;

        if (dist2 > CELL_SIZE * CELL_SIZE * 1.5f) {
            MoveTowards(npc, npc.warTargetPos, dt);
        } else {
            Vector2 pressurePos = {
                npc.warTargetPos.x + (float)((npc.id % 3) - 1) * CELL_SIZE * 0.9f,
                npc.warTargetPos.y + (float)(((npc.id / 3) % 3) - 1) * CELL_SIZE * 0.9f
            };
            MoveTowards(npc, pressurePos, dt);
        }

        return;
    }

    if (npc.leaderCaptainId != 0 && !npc.warAssigned) {
        NPC* cap = world.FindNpcById(npc.leaderCaptainId);

        if (!cap || !cap->alive) {
            npc.leaderCaptainId = 0;
            npc.inCombat = false;
        }
        else {
            // Leave combat if the leader is no longer attacking
            if (!cap->captainHasAttackOrder) {
                npc.inCombat = false;
            }
        }
    }

    if (npc.settlementId < 0 || npc.settlementId >= (int)world.settlements.size() ||
        !world.settlements[npc.settlementId].alive) {

        npc.wanderTimer -= dt;
        if (npc.wanderTimer <= 0.0f) {
            npc.wanderTimer = RandomFloat(1.0f, 2.5f);
            npc.wanderDir = SafeNormalizeEx(RandomUnit2D());
        }

        npc.vel = Vector2Scale(npc.wanderDir, npc.speed);
        npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));
        return;
    }

    const Settlement& s = world.settlements[npc.settlementId];

    // Follow the assigned captain when not in settlement war
    if (npc.leaderCaptainId != 0 && !npc.warAssigned) {
        NPC* cap = world.FindNpcById(npc.leaderCaptainId);

        if (!cap || !cap->alive || cap->humanRole != NPC::HumanRole::CAPTAIN) {
            npc.leaderCaptainId = 0;
            npc.formationSlot = -1;
        } else {
            const bool combatMode = cap->captainHasAttackOrder && cap->captainAttackGroupId != -1;

            if (combatMode) {
                int bi = FindNearestBanditInGroup(world, npc.pos, cap->captainAttackGroupId);

                if (bi != -1) {
                    NPC& b = world.npcs[bi];
                    const float meleeRange = 28.0f;
                    float d2 = Dist2(npc.pos, b.pos);

                    if (d2 > meleeRange * meleeRange) {
                        MoveTowards(npc, b.pos, dt, 1.15f);
                    } else {
                        npc.vel = {0, 0};
                        TryMeleeAttack(world, npc, b, dt, 0.80f);
                    }
                    return;
                }
            }

            Vector2 forward = CaptainFormation::GetCaptainFacing(*cap);
            Vector2 localOffset = CaptainFormation::GetSlotOffset(npc.formationSlot, combatMode);
            Vector2 worldOffset = RotateFromBaseY(localOffset, forward);
            Vector2 slotTarget = Vector2Add(cap->pos, worldOffset);

            float d2 = Dist2(npc.pos, slotTarget);
            if (d2 > 8.0f * 8.0f) {
                MoveTowards(npc, slotTarget, dt, 1.05f);
            } else {
                npc.vel = {0, 0};
            }
            return;
        }
    }

    // Handle free warrior combat near bandits
    const float ALERT_R  = 260.0f;
    const float GIVEUP_R = 340.0f;
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

    if (banditInAlert) {
        npc.inCombat = true;
        npc.combatTargetPos = world.npcs[nearestBandit].pos;
    } else if (npc.inCombat && !banditInGiveup) {
        npc.inCombat = false;
        npc.combatTargetPos = {0, 0};
        npc.attackCooldown = 0.0f;
    }

    if (npc.inCombat) {
        int nearestBandit = FindNearestBanditAny(world, npc.pos, GIVEUP_R);

        // Leave combat if no valid target remains
        if (nearestBandit == -1) {
            npc.inCombat = false;
            npc.attackCooldown = 0;
            npc.combatTargetPos = {0, 0};
            return;
        }

        const float meleeRange = 28.0f;
        float currentD2 = (nearestBandit != -1) ? Dist2(npc.pos, world.npcs[nearestBandit].pos) : FLT_MAX;

        if (nearestBandit != -1 && currentD2 > meleeRange * meleeRange) {
            MoveTowards(npc, npc.combatTargetPos, dt, 1.25f);
        } else {
            npc.vel = {0, 0};
        }

        if (nearestBandit != -1 && currentD2 <= meleeRange * meleeRange) {
            TryMeleeAttack(world, npc, world.npcs[nearestBandit], dt, 0.80f);
        }

        return;
    }

    // Return to camp behavior when idle
    Vector2 camp = s.campfirePosPx;
    if (camp.x == 0.0f && camp.y == 0.0f) camp = s.centerPx;

    if (!npc.formationAssigned) {
        float angle = RandomFloat(0.0f, 2.0f * PI);
        float radius = RandomFloat(20.0f, 50.0f);

        npc.formationOffset = { cosf(angle) * radius, sinf(angle) * radius };
        npc.formationAssigned = true;
    }

    Vector2 target = Vector2Add(camp, npc.formationOffset);

    float toT2 = Dist2(npc.pos, target);
    if (toT2 < 10.0f * 10.0f) {
        npc.wanderTimer -= dt;
        if (npc.wanderTimer <= 0.0f) {
            npc.wanderTimer = RandomFloat(0.6f, 1.4f);
            npc.wanderDir = SafeNormalizeEx(RandomUnit2D());
        }
        npc.vel = Vector2Scale(npc.wanderDir, npc.speed * 0.6f);
    } else {
        Vector2 dir = SafeNormalizeEx(Vector2Subtract(target, npc.pos));
        npc.vel = Vector2Scale(dir, npc.speed * 1.0f);
    }

    npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));
}

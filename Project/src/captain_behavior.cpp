#include "npc/captain_behavior.h"
#include "environment/world.h"
#include <algorithm>
#include <cfloat>
#include <vector>


uint32_t squadTargetId;

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

static inline bool PointInRectPx(const Rectangle& r, Vector2 p) {
    return (p.x >= r.x && p.x <= r.x + r.width &&
            p.y >= r.y && p.y <= r.y + r.height);
}

static Rectangle ExpandRect(const Rectangle& r, float e) {
    return Rectangle{ r.x - e, r.y - e, r.width + 2*e, r.height + 2*e };
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

namespace CaptainFormation {
    // Returns the captain-facing vector used by formation slots
    Vector2 GetCaptainFacing(const NPC& captain) {
        if (captain.captainHasMoveOrder) {
            Vector2 toTarget = Vector2Subtract(captain.captainMoveTarget, captain.pos);
            if (Vector2Length(toTarget) > 0.1f) return SafeNormalizeEx(toTarget);
        }

        if (Vector2Length(captain.vel) > 0.1f) {
            return SafeNormalizeEx(captain.vel);
        }

        return {0.0f, 1.0f};
    }

    // Returns the slot offset for march or combat formations
    Vector2 GetSlotOffset(int slot, bool combatMode) {
        if (slot < 0) slot = 0;
        if (slot > 14) slot = 14;

        if (!combatMode) {
            const float colX = 12.0f;
            const float rowY = 18.0f;

            if (slot < 14) {
                int row = slot / 2;
                int col = slot % 2;
                float x = (col == 0) ? -colX : colX;
                float y = -(row + 1) * rowY;
                return { x, y };
            }

            return { 0.0f, -8.0f * rowY };
        }

        static const Vector2 battleSlots[15] = {
                {-54.0f,  22.0f}, {-36.0f,  22.0f}, {-18.0f,  22.0f}, {  0.0f,  22.0f}, { 18.0f,  22.0f}, { 36.0f,  22.0f}, { 54.0f,  22.0f},
                {-36.0f,   4.0f}, {-18.0f,   4.0f}, {  0.0f,   4.0f}, { 18.0f,   4.0f}, { 36.0f,   4.0f},
                {-18.0f, -16.0f}, {  0.0f, -16.0f}, { 18.0f, -16.0f}
        };

        return battleSlots[slot];
    }

} // namespace CaptainFormation

static void MoveTowards(NPC& npc, Vector2 target, float dt, float speedMul = 1.0f) {
    Vector2 to = Vector2Subtract(target, npc.pos);
    if (Vector2Length(to) < 0.001f) {
        npc.vel = {0, 0};
        return;
    }

    Vector2 dir = SafeNormalizeEx(to);
    npc.vel = Vector2Scale(dir, npc.speed * speedMul);
    npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));
}

static int FindNearestBanditInRange(World& world, Vector2 from, float rangePx) {
    const float r2 = rangePx * rangePx;
    int best = -1;
    float bestD2 = FLT_MAX;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const auto& o = world.npcs[i];
        if (!o.alive) continue;
        if (o.humanRole != NPC::HumanRole::BANDIT) continue;

        float d2 = Dist2(o.pos, from);
        if (d2 <= r2 && d2 < bestD2) {
            bestD2 = d2;
            best = i;
        }
    }

    return best;
}

static int FindNearestBanditInGroup(World& world, Vector2 from, int groupId) {
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

static int FindThreatBanditNearSettlement(World& world, const Settlement& s, Vector2 from) {
    Rectangle threatRect = ExpandRect(s.boundsPx, 80.0f);

    int best = -1;
    float bestD2 = FLT_MAX;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const auto& o = world.npcs[i];
        if (!o.alive) continue;
        if (o.humanRole != NPC::HumanRole::BANDIT) continue;
        if (!PointInRectPx(threatRect, o.pos)) continue;

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

static void RefreshCaptainSquad(World& world, NPC& captain) {
    if (captain.settlementId < 0 || captain.settlementId >= (int)world.settlements.size()) return;
    if (!world.settlements[captain.settlementId].alive) return;

    struct Candidate {
        float d2;
        uint32_t id;
    };

    std::vector<Candidate> candidates;
    candidates.reserve(64);

    for (auto& n : world.npcs) {
        if (!n.alive) continue;
        if (n.humanRole != NPC::HumanRole::WARRIOR) continue;
        if (n.settlementId != captain.settlementId) continue;

        // Keep warriors attached to their current captain
        if (n.leaderCaptainId != 0 && n.leaderCaptainId != captain.id) continue;

        candidates.push_back({ Dist2(n.pos, captain.pos), n.id });
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) { return a.d2 < b.d2; });

    std::vector<uint32_t> chosen;
    chosen.reserve(15);

    for (int i = 0; i < (int)candidates.size() && (int)chosen.size() < 15; i++) {
        chosen.push_back(candidates[i].id);
    }

    for (auto& n : world.npcs) {
        if (!n.alive) continue;
        if (n.humanRole != NPC::HumanRole::WARRIOR) continue;
        if (n.leaderCaptainId != captain.id) continue;

        bool stillChosen = false;
        for (uint32_t cid : chosen) {
            if (cid == n.id) {
                stillChosen = true;
                break;
            }
        }

        if (!stillChosen) {
            n.leaderCaptainId = 0;
            n.formationSlot = -1;
        }
    }

    for (int slot = 0; slot < (int)chosen.size(); slot++) {
        NPC* w = world.FindNpcById(chosen[slot]);
        if (!w) continue;
        w->leaderCaptainId = captain.id;
        w->formationSlot = slot;
    }
}

static bool ExecuteAttackOrder(World& world, NPC& captain, float dt, int groupId) {
    int bi = FindNearestBanditInGroup(world, captain.pos, groupId);
    if (bi == -1) return false;

    NPC& b = world.npcs[bi];
    captain.captainAttackTargetId = b.id;

    const float meleeRange = 28.0f;
    float d2 = Dist2(captain.pos, b.pos);

    if (d2 > meleeRange * meleeRange) {
        MoveTowards(captain, b.pos, dt, 1.0f);
    } else {
        captain.vel = {0, 0};
        TryMeleeAttack(world, captain, b, dt, 0.75f);
    }

    return true;
}

// Updates captain behavior
void CaptainBehavior::Update(World& world, NPC& npc, float dt) {
    if (npc.humanRole != NPC::HumanRole::CAPTAIN) return;

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

    RefreshCaptainSquad(world, npc);

    // Handle player-issued attack orders
    if (npc.captainHasAttackOrder && npc.captainAttackGroupId != -1) {
        if (ExecuteAttackOrder(world, npc, dt, npc.captainAttackGroupId)) {
            return;
        }

        npc.captainHasAttackOrder = false;
        npc.captainAttackGroupId = -1;
        npc.captainAttackTargetId = 0;
    }

    // Handle settlement war and defense behavior
    if (npc.warAssigned &&
        npc.warTargetSettlementId >= 0 &&
        world.IsSettlementAliveAndValid(npc.warTargetSettlementId) &&
        !npc.captainHasMoveOrder &&
        !npc.captainHasAttackOrder)
    {
        if (npc.warIsDefender &&
            npc.settlementId >= 0 &&
            npc.settlementId < (int)world.settlements.size())
        {
            npc.warTargetPos = world.settlements[npc.settlementId].centerPx;
        }
        else {
            npc.warTargetPos = world.settlements[npc.warTargetSettlementId].centerPx;
        }

        // Captain stops holding formation and just fights nearby enemy combat units
        if (npc.warInBattle) {
            int localEnemyIndex = FindNearestEnemyCombatNearNpc(world, npc, CELL_SIZE * 9.0f);
            if (localEnemyIndex != -1) {
                NPC& enemy = world.npcs[localEnemyIndex];

                float dx = enemy.pos.x - npc.pos.x;
                float dy = enemy.pos.y - npc.pos.y;
                float d2 = dx*dx + dy*dy;

                if (d2 <= 18.0f * 18.0f) {
                    TryMeleeAttack(world, npc, enemy, dt, 0.85f);
                } else {
                    MoveTowards(npc, enemy.pos, dt);
                }
                return;
            }

            Vector2 searchPos = {
                npc.pos.x + CELL_SIZE * 0.5f,
                npc.pos.y - CELL_SIZE * 0.5f
            };
            MoveTowards(npc, searchPos, dt, 0.8f);
            return;
        }

        // Approach / regroup mode
        int enemyIndex = FindNearestEnemyNpcForSettlementWar(world, npc, npc.warTargetSettlementId, CELL_SIZE * 26.0f);
        if (enemyIndex != -1) {
            NPC& enemy = world.npcs[enemyIndex];

            float dx = enemy.pos.x - npc.pos.x;
            float dy = enemy.pos.y - npc.pos.y;
            float d2 = dx*dx + dy*dy;

            if (d2 <= 18.0f * 18.0f) {
                TryMeleeAttack(world, npc, enemy, dt, 0.85f);
            } else {
                MoveTowards(npc, enemy.pos, dt);
            }

            return;
        }

        // If no enemy combat troops remain, attack the barracks
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
                        npc.attackCooldown = 0.85f;
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
                npc.warTargetPos.x + CELL_SIZE * 0.5f,
                npc.warTargetPos.y - CELL_SIZE * 0.5f
            };
            MoveTowards(npc, pressurePos, dt);
        }

        return;
    }

    // Handle direct manual control
    if (!npc.captainAutoMode) {
        if (npc.captainHasMoveOrder) {
            float d2 = Dist2(npc.pos, npc.captainMoveTarget);
            if (d2 < 10.0f * 10.0f) {
                npc.captainHasMoveOrder = false;
                npc.manualControl = false;
                npc.hasMoveTarget = false;
                npc.vel = {0, 0};
            } else {
                MoveTowards(npc, npc.captainMoveTarget, dt, 1.0f);
            }
        } else {
            npc.manualControl = false;
            npc.hasMoveTarget = false;
            npc.vel = {0, 0};
        }

        int nearBandit = FindNearestBanditInRange(world, npc.pos, 24.0f);
        if (nearBandit != -1) {
            TryMeleeAttack(world, npc, world.npcs[nearBandit], dt, 0.75f);
        }

        return;
    }

    // Handle automatic threat response
    int threatBandit = FindThreatBanditNearSettlement(world, s, npc.pos);
    if (threatBandit != -1) {
        npc.captainHasAttackOrder = true;
        npc.captainAttackGroupId = world.npcs[threatBandit].banditGroupId;
        npc.captainAttackTargetId = world.npcs[threatBandit].id;

        ExecuteAttackOrder(world, npc, dt, npc.captainAttackGroupId);
        return;
    }

    // Idle near the campfire when no threat is present
    Vector2 camp = s.campfirePosPx;
    if (camp.x == 0.0f && camp.y == 0.0f) camp = s.centerPx;

    if (Dist2(npc.pos, camp) > 16.0f * 16.0f) {
        MoveTowards(npc, camp, dt, 0.75f);
    } else {
        npc.vel = {0, 0};
    }
}

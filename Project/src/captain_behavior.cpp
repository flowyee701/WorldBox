#include "npc/captain_behavior.h"
#include "environment/world.h"
#include <cfloat>
#include <vector>
#include <algorithm>

static inline float Dist2(Vector2 a, Vector2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx*dx + dy*dy;
}

static inline bool PointInRectPx(const Rectangle& r, Vector2 p) {
    return (p.x >= r.x && p.x <= r.x + r.width &&
            p.y >= r.y && p.y <= r.y + r.height);
}

static Rectangle ExpandRect(const Rectangle& r, float e) {
    return Rectangle{ r.x - e, r.y - e, r.width + 2*e, r.height + 2*e };
}

// 15 слотов: 3 ряда по 5 (боевое построение "вперёд")
static void BuildAttackSlots(std::vector<Vector2>& outSlots) {
    outSlots.clear();
    outSlots.reserve(15);

    // локально: +Y вперёд (потом повернём по направлению к цели)
    // шаги можно подкрутить под CELL_SIZE, но оставим в пикселях
    const float dx = 22.0f;
    const float dy = 18.0f;

    for (int row = 0; row < 3; row++) {
        float y = 20.0f + row * dy;
        for (int col = 0; col < 5; col++) {
            float x = (col - 2) * dx;
            outSlots.push_back(Vector2{ x, y });
        }
    }
}

static Vector2 Rotate(Vector2 v, float ang) {
    float c = cosf(ang), s = sinf(ang);
    return Vector2{ v.x * c - v.y * s, v.x * s + v.y * c };
}

static void RebuildSquad(World& world, NPC& captain) {
    if (captain.settlementId < 0 || captain.settlementId >= (int)world.settlements.size()) return;
    if (!world.settlements[captain.settlementId].alive) return;

    // собрать кандидатов: воины этого поселения
    struct Cand { float d2; uint32_t id; };
    std::vector<Cand> cands;
    cands.reserve(64);

    for (auto& n : world.npcs) {
        if (!n.alive) continue;
        if (n.humanRole != NPC::HumanRole::WARRIOR) continue;
        if (n.settlementId != captain.settlementId) continue;

        float d2 = Dist2(n.pos, captain.pos);
        cands.push_back(Cand{ d2, n.id });
    }

    std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b){ return a.d2 < b.d2; });

    // помечаем топ-15
    const int maxSquad = 15;
    std::vector<uint32_t> chosen;
    for (int i = 0; i < (int)cands.size() && (int)chosen.size() < maxSquad; i++) {
        chosen.push_back(cands[i].id);
    }

    // снять тех, кто больше не входит в отряд этого капитана
    for (auto& n : world.npcs) {
        if (!n.alive) continue;
        if (n.humanRole != NPC::HumanRole::WARRIOR) continue;
        if (n.leaderCaptainId != captain.id) continue;

        bool still = false;
        for (uint32_t cid : chosen) {
            if (cid == n.id) { still = true; break; }
        }
        if (!still) {
            n.leaderCaptainId = 0;
            n.formationSlot = -1;
        }
    }

    // назначить слоты выбранным
    for (int i = 0; i < (int)chosen.size(); i++) {
        NPC* w = world.FindNpcById(chosen[i]);
        if (!w) continue;
        w->leaderCaptainId = captain.id;
        w->formationSlot = i; // 0..14
    }
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

static void TryMeleeAttack(NPC& attacker, NPC& target, float dt, float cooldownSeconds) {
    attacker.attackCooldown -= dt;
    if (attacker.attackCooldown > 0.0f) return;

    target.hp -= attacker.damage;
    attacker.attackCooldown = cooldownSeconds;

    if (target.hp <= 0.0f) {
        target.hp = 0.0f;
        target.alive = false;
    }
}

void CaptainBehavior::Update(World& world, NPC& npc, float dt) {
    if (npc.humanRole != NPC::HumanRole::CAPTAIN) return;

    // если поселение потеряно — как воин: блуждать
    if (npc.settlementId < 0 || npc.settlementId >= (int)world.settlements.size() ||
        !world.settlements[npc.settlementId].alive) {

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
    // Всегда обновляем отряд, если капитан привязан к поселению
    RebuildSquad(world, npc);

    // =========================================================
    // PLAYER ATTACK ORDER: kill whole bandit group
    // =========================================================
    if (npc.captainHasAttackOrder && npc.captainAttackGroupId != -1) {
        int bi = FindNearestBanditInGroup(world, npc.pos, npc.captainAttackGroupId);

        // если группа закончилась — сброс
        if (bi == -1) {
            npc.captainHasAttackOrder = false;
            npc.captainAttackGroupId = -1;
            npc.captainAttackTargetId = 0;
        } else {
            NPC& b = world.npcs[bi];
            Vector2 target = b.pos;

            // движение к цели
            Vector2 dir = SafeNormalize(Vector2Subtract(target, npc.pos));
            npc.vel = Vector2Scale(dir, npc.speed * 1.35f);
            npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));

            // атака в мили
            const float meleeRange = 28.0f;
            if (Dist2(npc.pos, target) <= meleeRange * meleeRange) {
                TryMeleeAttack(npc, b, dt, 0.80f);
            }
            return;
        }
    }


    // 2) РУЧНОЙ РЕЖИМ: капитан слушается ТОЛЬКО игрока
    // (никаких угроз/авто-атаки, пока игрок не включит AUTO обратно)
    if (!npc.captainAutoMode) {
        if (npc.captainHasMoveOrder) {
            Vector2 target = npc.captainMoveTarget;

            float d2 = Dist2(npc.pos, target);
            if (d2 < 10.0f * 10.0f) {
                npc.captainHasMoveOrder = false;
                npc.vel = {0, 0};
            } else {
                Vector2 dir = SafeNormalize(Vector2Subtract(target, npc.pos));
                npc.vel = Vector2Scale(dir, npc.speed); // скорость капитана = npc.speed
                npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));
            }
        } else {
            // без приказа — стоит (или можно слегка держаться у костра, но ты просил слушаться игрока)
            npc.vel = {0, 0};
        }
        // MANUAL: не преследуем врага, но если он рядом — капитан отбивается
        const float meleeRange = 20.0f;
        int nearBandit = FindNearestBanditInRange(world, npc.pos, meleeRange);
        if (nearBandit != -1) {
            NPC& b = world.npcs[nearBandit];
            TryMeleeAttack(npc, b, dt, 0.80f);
        }
        return;
    }

    // 3) AUTO режим: реагируем только если есть угроза поселению (bandit в пределах bounds +/- расширение)
    const Rectangle threatRect = ExpandRect(s.boundsPx, 60.0f);

    int nearestBandit = -1;
    float nearestD2 = FLT_MAX;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const auto& other = world.npcs[i];
        if (!other.alive) continue;
        if (other.humanRole != NPC::HumanRole::BANDIT) continue;
        if (!PointInRectPx(threatRect, other.pos)) continue;

        float d2 = Dist2(other.pos, npc.pos);
        if (d2 < nearestD2) {
            nearestD2 = d2;
            nearestBandit = i;
        }
    }

    if (nearestBandit != -1) {
        Vector2 target = world.npcs[nearestBandit].pos;
        // AUTO: если дошли до дистанции удара — атакуем
        const float meleeRange = 20.0f;
        if (Dist2(npc.pos, target) <= meleeRange * meleeRange) {
            NPC& b = world.npcs[nearestBandit];
            TryMeleeAttack(npc, b, dt, 0.80f);
        }

        float d2 = Dist2(npc.pos, target);
        if (d2 < 10.0f * 10.0f) {
            npc.vel = {0, 0};
        } else {
            Vector2 dir = SafeNormalize(Vector2Subtract(target, npc.pos));
            npc.vel = Vector2Scale(dir, npc.speed);
            npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));
        }
        return;
    }

    // 4) AUTO без угрозы: idle у костра
    {
        Vector2 camp = s.centerPx;
        Vector2 dir = SafeNormalize(Vector2Subtract(camp, npc.pos));
        npc.vel = Vector2Scale(dir, npc.speed * 0.6f);
        npc.pos = Vector2Add(npc.pos, Vector2Scale(npc.vel, dt));
    }
}
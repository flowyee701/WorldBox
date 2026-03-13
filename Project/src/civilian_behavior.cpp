#include "npc/civilian_behavior.h"
#include "environment/world.h"
#include <cmath>

// Returns the length of a 2D vector
static float Length(Vector2 v) {
    return sqrtf(v.x*v.x + v.y*v.y);
}

// Picks a random tile center inside a settlement
static Vector2 RandomPointInSettlement(const World& world, const Settlement& s) {
    if (s.tiles.empty()) {
        return {
                RandomFloat(0, world.worldW),
                RandomFloat(0, world.worldH)
        };
    }

    int index = GetRandomValue(0, (int)s.tiles.size() - 1);
    auto it = s.tiles.begin();
    std::advance(it, index);

    int tile = *it;
    int cx = tile % world.cols;
    int cy = tile / world.cols;

    return {
            (cx + 0.5f) * CELL_SIZE,
            (cy + 0.5f) * CELL_SIZE
    };
}

// Updates civilian wandering behavior
void CivilianBehavior::Update(World& world, NPC& npc, float dt)
{
    if (dt <= 0.0f) return;

    const float MAX_SPEED = npc.speed * 1.4f;
    const float SLOW_RADIUS = 25.0f;
    const float STOP_RADIUS = 6.0f;

    // Refresh settlement ownership if needed
    if (npc.settlementId >= 0) {
        if (npc.settlementId >= (int)world.settlements.size() ||
            !world.settlements[npc.settlementId].alive)
        {
            npc.settlementId = -1;
            npc.hasRoamTarget = false;
        }
    }

    if (npc.settlementId == -1) {
        for (int i = 0; i < (int)world.settlements.size(); i++) {
            if (!world.settlements[i].alive) continue;
            if (PointInSettlementPx(world.settlements[i], npc.pos)) {
                npc.settlementId = i;
                npc.hasRoamTarget = false;
                break;
            }
        }
    }

    // Stay idle briefly after reaching a target
    if (npc.restTimer > 0.0f) {
        npc.restTimer -= dt;
        npc.vel.x *= 0.85f;
        npc.vel.y *= 0.85f;
        return;
    }

    // Pick a new roam target when needed
    if (!npc.hasRoamTarget) {
        if (npc.settlementId == -1) {
            npc.roamTarget = {
                    RandomFloat(0, world.worldW),
                    RandomFloat(0, world.worldH)
            };
        } else {
            npc.roamTarget =
                    RandomPointInSettlement(world,
                                            world.settlements[npc.settlementId]);
        }

        npc.hasRoamTarget = true;
    }

    // Move toward the current roam target
    Vector2 toTarget = {
            npc.roamTarget.x - npc.pos.x,
            npc.roamTarget.y - npc.pos.y
    };

    float dist = Length(toTarget);

    if (dist < STOP_RADIUS) {
        npc.hasRoamTarget = false;
        npc.restTimer = RandomFloat(0.2f, 0.6f);
        return;
    }

    Vector2 dir = SafeNormalize(toTarget);

    float desiredSpeed = MAX_SPEED;

    if (dist < SLOW_RADIUS) {
        desiredSpeed = MAX_SPEED * (dist / SLOW_RADIUS);
    }

    Vector2 desiredVel = {
            dir.x * desiredSpeed,
            dir.y * desiredSpeed
    };

    npc.vel.x = npc.vel.x * 0.6f + desiredVel.x * 0.4f;
    npc.vel.y = npc.vel.y * 0.6f + desiredVel.y * 0.4f;

    npc.pos.x += npc.vel.x * dt;
    npc.pos.y += npc.vel.y * dt;

    // Keep civilians inside the world bounds
    npc.pos.x = Clamp(npc.pos.x, 0.0f, (float)world.worldW);
    npc.pos.y = Clamp(npc.pos.y, 0.0f, (float)world.worldH);
}

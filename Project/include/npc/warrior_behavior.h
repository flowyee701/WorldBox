#pragma once
#include "npc/npc.h"

struct World; // forward declaration

struct WarriorBehavior {
    static void Update(World& world, NPC& npc, float dt);
};
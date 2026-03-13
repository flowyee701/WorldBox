#pragma once
#include "npc.h"

struct World; // forward

struct HumanBehavior {
    static void Update(World& world, NPC& npc, float dt);
};
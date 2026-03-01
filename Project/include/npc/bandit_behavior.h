#pragma once
#include "npc.h"

struct World; // forward declaration

struct BanditBehavior {
    static void Update(World& world, NPC& npc, float dt);
};
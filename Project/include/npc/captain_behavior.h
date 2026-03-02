#pragma once
#include "npc.h"

struct World;

struct CaptainBehavior {
    static void Update(World& world, NPC& npc, float dt);
};
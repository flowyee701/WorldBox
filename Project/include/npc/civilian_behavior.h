#pragma once
#include "npc.h"

struct World;

struct CivilianBehavior {
    static void Update(World& world, NPC& npc, float dt);
};
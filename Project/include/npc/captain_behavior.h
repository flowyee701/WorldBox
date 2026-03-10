#pragma once
#include "npc.h"

struct World;

namespace CaptainFormation {
    Vector2 GetCaptainFacing(const NPC& captain);
    Vector2 GetSlotOffset(int slot, bool combatMode);
}

struct CaptainBehavior {
    static void Update(World& world, NPC& npc, float dt);
};
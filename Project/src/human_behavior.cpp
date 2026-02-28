#include "npc/human_behavior.h"
#include "environment/world.h"
#include "npc/civilian_behavior.h"
#include "npc/warrior_behavior.h"
#include "npc/bandit_behavior.h"

void HumanBehavior::Update(World& world, NPC& npc, float dt) {
    switch (npc.humanRole) {
        case NPC::HumanRole::CIVILIAN:
            CivilianBehavior::Update(world, npc, dt);
            break;

        case NPC::HumanRole::WARRIOR:
            WarriorBehavior::Update(world, npc, dt);
            break;

        case NPC::HumanRole::BANDIT:
            BanditBehavior::Update(world, npc, dt);
            break;

        default:
            break;
    }
}
#include "human_behavior.h"
#include "world.h"
#include "civilian_behavior.h"
#include "warrior_behavior.h"
#include "bandit_behavior.h"

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
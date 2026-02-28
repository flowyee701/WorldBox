#include "../include/human_behavior.h"
#include "../include/world.h"
#include "../include/civilian_behavior.h"
#include "../include/warrior_behavior.h"
#include "../include/bandit_behavior.h"

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
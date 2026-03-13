#pragma once
#include <raylib.h>
#include <cstdint>

struct NPC {
    // Transform state
    Vector2 pos{0, 0};
    Vector2 vel{0, 0};

    // Identity
    enum class Type { HUMAN, ANIMAL } type = Type::HUMAN;

    enum class HumanRole { NONE, CIVILIAN, WARRIOR, BANDIT, CAPTAIN }
            humanRole = HumanRole::CIVILIAN;

    enum class WarriorRank { WARRIOR, CAPTAIN };
    WarriorRank warriorRank = WarriorRank::WARRIOR;
    bool isCaptain = false;

    int settlementId = -1;
    bool alive = true;

    uint16_t skinId = 0;

    // Shared formation helpers
    bool hasFormationOffset = false;
    Vector2 formationOffset{0, 0};
    bool formationAssigned = false;

    // Stats
    float speed  = 15.0f;
    float hp     = 100.0f;
    float damage = 10.0f;

    // Generic movement
    Vector2 wanderDir = {0.0f, 0.0f};
    float wanderTimer = 0.0f;
    Vector2 wanderTarget{0, 0};
    int homeTile = -1;

    bool isIdle = false;
    float idleTimer = 0.0f;
    float moveTimer = 0.0f;

    // Combat
    float attackCooldown = 0.0f;

    // Melee attack animation
    bool isAttacking = false;
    float attackAnimTimer = 0.0f;
    float attackAnimDuration = 0.16f;
    Vector2 attackAnimDir{0.0f, 1.0f};

    // Warrior formation state
    bool inCombat = false;
    Vector2 combatTargetPos = {0.0f, 0.0f};
    int squadId = -1;

    // Stable identity for squads and selection
    uint32_t id = 0;

    // Captain squad links for warrior followers
    uint32_t leaderCaptainId = 0;
    int formationSlot = -1;

    // Captain player commands
    bool manualControl = false;
    bool hasMoveTarget = false;
    Vector2 moveTargetPx{0, 0};

    // Bandit group state
    int banditGroupId = -1;
    Vector2 banditGroupDir = {0.0f, 0.0f};
    float banditLifeTime = 0.0f;

    // Shared roaming target
    Vector2 roamTarget = {0.0f, 0.0f};
    bool hasRoamTarget = false;
    float restTimer = 0.0f;

    // Captain control state
    bool captainAutoMode = true;
    bool captainHasMoveOrder = false;
    Vector2 captainMoveTarget{0, 0};

    // Captain attack order state
    bool captainHasAttackOrder = false;
    int captainAttackGroupId = -1;
    uint32_t captainAttackTargetId = 0;

    // Death animation state
    bool isDying = false;
    float deathTimer = 0.0f;
    float deathDuration = 0.65f;

    // Settlement war state
    bool warAssigned = false;
    int warFromSettlementId = -1;
    int warTargetSettlementId = -1;
    bool warMarching = false;
    Vector2 warTargetPos{0.0f, 0.0f};

    // Tree chopping state
    bool choppingTree = false;
    float chopTimer = 0.0f;

    // Squad formation links during settlement war
    uint32_t warCaptainId = 0;
    int warSquadIndex = -1;
    bool warIsDefender = false;
    bool warReady = false;
    bool warInBattle = false;
    float warBattleLockTimer = 0.0f;
};

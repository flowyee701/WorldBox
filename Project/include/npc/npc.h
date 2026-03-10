#pragma once
#include <raylib.h>
#include <cstdint>

struct NPC {

    // -------------------------------------------------
    // Transform
    // -------------------------------------------------
    Vector2 pos{0, 0};
    Vector2 vel{0, 0};


    // -------------------------------------------------
    // Identity
    // -------------------------------------------------
    enum class Type { HUMAN, ANIMAL } type = Type::HUMAN;

    enum class HumanRole { NONE, CIVILIAN, WARRIOR, BANDIT, CAPTAIN }
            humanRole = HumanRole::CIVILIAN;

    enum class WarriorRank { WARRIOR, CAPTAIN };
    WarriorRank warriorRank = WarriorRank::WARRIOR;
    // --- warriors ---
    bool isCaptain = false;

    int settlementId = -1;   // -1 = дикий / непривязанный
    bool alive = true;

    uint16_t skinId = 0;     // на будущее (скины)

    // npc.h (внутри struct NPC)
    bool hasFormationOffset = false;   // <-- чтобы warrior_behavior.cpp собрался
    Vector2 formationOffset{0,0};      // обычно рядом с этим используется
    bool formationAssigned = false; // на будущее (если код где-то использует это имя)

    // -------------------------------------------------
    // Stats
    // -------------------------------------------------
    float speed  = 15.0f;
    float hp     = 100.0f;
    float damage = 10.0f;

    // -------------------------------------------------
    // Generic movement (wander / idle)
    // -------------------------------------------------
    Vector2 wanderDir = {0.0f, 0.0f};
    float wanderTimer = 0.0f;
    Vector2 wanderTarget{0,0};
    int homeTile = -1;

    bool isIdle = false;
    float idleTimer = 0.0f;
    float moveTimer = 0.0f;

    // -------------------------------------------------
    // Combat
    // -------------------------------------------------
    float attackCooldown = 0.0f;

    // -------------------------------------------------
    // Formation / squad (WARRIOR)
    // -------------------------------------------------
    bool inCombat = false;

    Vector2 combatTargetPos = {0.0f, 0.0f};


    int squadId = -1;                 // для воинов (отряд)

    // -------------------------------------------------
    // Stable identity (for squads / selection)
    // -------------------------------------------------
    uint32_t id = 0;                  // 0 = invalid, valid ids start from 1

    // -------------------------------------------------
    // Captain squad linking (for WARRIOR followers)
    // -------------------------------------------------
    uint32_t leaderCaptainId = 0;     // 0 = no leader
    int formationSlot = -1;           // 0..14 for squad members

    // -------------------------------------------------
    // Captain player commands
    // -------------------------------------------------
    bool manualControl = false;       // true if player issued move
    bool hasMoveTarget = false;
    Vector2 moveTargetPx{0, 0};

    // -------------------------------------------------
    // Bandit group
    // -------------------------------------------------
    int banditGroupId = -1;
    Vector2 banditGroupDir = {0.0f, 0.0f};
    float banditLifeTime = 0.0f;

    // --- roaming target (for civilians / warriors) ---
    Vector2 roamTarget = {0.0f, 0.0f};
    bool hasRoamTarget = false;
    float restTimer = 0.0f;
    // -------------------------------------------------
    // Captain control (player / auto)
    // -------------------------------------------------
    bool captainAutoMode = true;      // true = реагирует на угрозу (авто), false = слушает только игрока
    bool captainHasMoveOrder = false; // активен ли приказ движения от игрока
    Vector2 captainMoveTarget{0, 0};  // куда идти по приказу
    // -------------------------------------------------
    // Captain: attack order (player designated target/group)
    // -------------------------------------------------
    bool captainHasAttackOrder = false;
    int  captainAttackGroupId = -1;     // banditGroupId
    uint32_t captainAttackTargetId = 0; // optional: specific bandit id (can die)

    // -------------------------------------------------
    // Death animation state
    // -------------------------------------------------
    bool isDying = false;
    float deathTimer = 0.0f;
    float deathDuration = 0.65f;
};

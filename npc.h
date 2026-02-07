#pragma once
#include "raylib.h"
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

    enum class HumanRole { NONE, CIVILIAN, WARRIOR, BANDIT }
            humanRole = HumanRole::CIVILIAN;

    int settlementId = -1;   // -1 = дикий / непривязанный
    bool alive = true;

    uint16_t skinId = 0;     // на будущее (скины)

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

    Vector2 formationOffset = {0.0f, 0.0f};
    bool formationAssigned = false;

    int squadId = -1;                 // для воинов (отряд)

    // -------------------------------------------------
    // Bandit group
    // -------------------------------------------------
    int banditGroupId = -1;
    Vector2 banditGroupDir = {0.0f, 0.0f};
    float banditLifeTime = 0.0f;
};
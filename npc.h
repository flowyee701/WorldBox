#pragma once
#include "raylib.h"
#include <cstdint>

struct NPC {
    Vector2 pos{};
    Vector2 vel{};

    int settlementId = -1;
    // --- formation ---
    Vector2 formationOffset = {0.0f, 0.0f};
    bool formationAssigned = false;

    float speed  = 15.0f;
    float health = 100.0f;
    float banditLifeTime = 0.0f; // сколько секунд бандит живёт
    // --- combat ---
    float hp = 100.0f;
    float damage = 10.0f;
    bool alive = true;
    // --- combat cooldown ---
    float attackCooldown = 0.0f;   // сколько осталось до следующей атаки

    enum class Type { HUMAN, ANIMAL } type = Type::HUMAN;

    enum class HumanRole { NONE, CIVILIAN, WARRIOR, BANDIT } humanRole = HumanRole::CIVILIAN;

    uint16_t skinId = 0; // для будущих скинов
    // --- warrior group behaviour ---
    Vector2 formationDir = {0.0f, 0.0f}; // общее направление группы

    Vector2 wanderDir = {0, 0};
    // --- bandit group data ---
    int banditGroupId = -1;
    Vector2 banditGroupDir = {0.0f, 0.0f};
    // --- walk / idle state (for civilians / warriors) ---
    float idleTimer = 0.0f;        // сколько ещё стоим
    float moveTimer = 0.0f;        // сколько ещё идём
    bool isIdle = false;
};
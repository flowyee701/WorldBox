// src/Animal.cpp
#include "npc/Animal.h"
#include "terrain/terrain.h"
#include <cmath>
#include "raymath.h"

Animal::Animal(Vector2 pos) : position(pos), speed(DEFAULT_SPEED), hunger(100.0f) {
    velocity = { (float)GetRandomValue(-10, 10) / 10.0f, (float)GetRandomValue(-10, 10) / 10.0f };
}

void Animal::Wander(float deltaTime, const Terrain* terrain) {
    Vector2 newPos = position;
    if (GetRandomValue(0, 100) < 2) {
        velocity.x += (float)GetRandomValue(-5, 5) / 10.0f;
        velocity.y += (float)GetRandomValue(-5, 5) / 10.0f;

        if (Vector2Length(velocity) > 0) {
            velocity = Vector2Normalize(velocity);
        }
    }

    // 2. Рассчитываем новую позицию
    newPos.x += velocity.x * speed * deltaTime;
    newPos.y += velocity.y * speed * deltaTime;

    // Проверяем, можно ли ходить по новой позиции (не вода)
    bool canWalk = true;
    if (terrain) {
        canWalk = terrain->canWalk(newPos.x, newPos.y);
    }

    // Если место непроходимое (вода), отражаем скорость и не двигаемся
    if (!canWalk) {
        velocity.x *= -1;
        velocity.y *= -1;
    } else {
        position = newPos;
    }

    // Проверка границ
    if (position.x < 0) { position.x = 0; velocity.x *= -1; }
    else if (position.x > 1400) { position.x = 1400; velocity.x *= -1; }
    if (position.y < 0) { position.y = 0; velocity.y *= -1; }
    else if (position.y > 900) { position.y = 900; velocity.y *= -1; }
}

void Animal::Update(float deltaTime, const Terrain* terrain) {
    Wander(deltaTime, terrain);
    hunger -= HUNGER_DECAY_RATE * deltaTime;
}

void Animal::Draw() const {
    // ... без изменений (как было)
    if (textureLoaded) {
        float desiredWidth = 32.0f;
        float desiredHeight = 32.0f;
        float flip = (velocity.x < 0) ? -1.0f : 1.0f;
        Rectangle src = { 0.0f, 0.0f, (float)texture.width * flip, (float)texture.height };
        Rectangle dst = { position.x, position.y, desiredWidth, desiredHeight };
        Vector2 origin = { desiredWidth / 2.0f, desiredHeight };
        DrawTexturePro(texture, src, dst, origin, 0.0f, WHITE);
    } else {
        DrawRectangleV({position.x - 5, position.y - 5}, {10, 10}, GOLD);
    }
}

Texture2D Animal::texture = { 0 };
bool Animal::textureLoaded = false;
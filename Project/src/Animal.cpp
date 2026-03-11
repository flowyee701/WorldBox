#include "npc/Animal.h"
#include <cmath>

Animal::Animal(Vector2 pos) : position(pos), speed(50.0f), hunger(100.0f) {
    velocity = { (float)GetRandomValue(-10, 10) / 10.0f, (float)GetRandomValue(-10, 10) / 10.0f };
}

void Animal::Wander(float deltaTime) {
    // Случайное изменение направления
    if (GetRandomValue(0, 100) < 2) {
        velocity.x += (float)GetRandomValue(-5, 5) / 10.0f;
        velocity.y += (float)GetRandomValue(-5, 5) / 10.0f;
    }

    position.x += velocity.x * speed * deltaTime;
    position.y += velocity.y * speed * deltaTime;
    if (position.x < 0) { position.x = 0; velocity.x *= -1; }
    if (position.x > 1400) { position.x = 1400; velocity.x *= -1; }
    if (position.y < 0) { position.y = 0; velocity.y *= -1; }
    if (position.y > 900) { position.y = 900; velocity.y *= -1; }
}

void Animal::Update(float deltaTime) {
    Wander(deltaTime);
    hunger -= 1.0f * deltaTime;
}

void Animal::Draw() const {
    DrawRectangleV(position, {10, 10}, GOLD);
}
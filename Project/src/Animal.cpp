#include "npc/Animal.h"
#include <cmath>
#include "raymath.h"

Animal::Animal(Vector2 pos) : position(pos), speed(10.0f), hunger(100.0f) {
    velocity = { (float)GetRandomValue(-10, 10) / 10.0f, (float)GetRandomValue(-10, 10) / 10.0f };
}

void Animal::Wander(float deltaTime) {
    // 1. Случайное изменение направления
    if (GetRandomValue(0, 100) < 2) {
        velocity.x += (float)GetRandomValue(-5, 5) / 10.0f;
        velocity.y += (float)GetRandomValue(-5, 5) / 10.0f;

        // Ограничиваем скорость, чтобы не разгонялись
        if (Vector2Length(velocity) > 0) {
            velocity = Vector2Normalize(velocity);
        }
    }

    // 2. Движение
    position.x += velocity.x * speed * deltaTime;
    position.y += velocity.y * speed * deltaTime;

    // ---  Проверка границ (чтобы не пропадали) ---
    if (position.x < 0) {
        position.x = 0;
        velocity.x *= -1;
    } else if (position.x > 1400) {
        position.x = 1400;
        velocity.x *= -1;
    }

    if (position.y < 0) {
        position.y = 0;
        velocity.y *= -1;
    } else if (position.y > 900) {
        position.y = 900;
        velocity.y *= -1;
    }
}

void Animal::Update(float deltaTime) {
    Wander(deltaTime);
    hunger -= 1.0f * deltaTime;
}

void Animal::Draw() const {
    if (textureLoaded) {
        // --- НАСТРОЙКА РАЗМЕРА ---
        // Установи здесь размер в пикселях, который тебе нравится
        // Например, 30.0f x 30.0f (чуть больше одной клетки земли)
        float desiredWidth = 32.0f;
        float desiredHeight = 32.0f;

        // Логика разворота (Flip)
        float flip = (velocity.x < 0) ? -1.0f : 1.0f;

        // Источник (вся картинка из файла)
        Rectangle src = {
                0.0f,
                0.0f,
                (float)texture.width * flip,
                (float)texture.height
        };

        // Назначение (размер на экране)
        Rectangle dst = {
                position.x,
                position.y,
                desiredWidth,
                desiredHeight
        };

        // Точка привязки (центр копыт)
        Vector2 origin = { desiredWidth / 2.0f, desiredHeight };

        DrawTexturePro(texture, src, dst, origin, 0.0f, WHITE);

    } else {
        // Если текстура не загружена, рисуем маленький желтый квадрат
        DrawRectangleV({position.x - 5, position.y - 5}, {10, 10}, GOLD);
    }
}
Texture2D Animal::texture = { 0 };
bool Animal::textureLoaded = false;
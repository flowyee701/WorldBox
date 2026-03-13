// include/npc/Animal.h
#ifndef ANIMAL_H
#define ANIMAL_H

#include "raylib.h"
#include "environment/WorldObject.h"   // <-- путь к базовому классу

class Animal : public WorldObject {   // <-- наследование
public:
    // constexpr константы
    static constexpr float DEFAULT_SPEED = 10.0f;
    static constexpr float HUNGER_DECAY_RATE = 1.0f;

    Vector2 position;
    Vector2 velocity;
    float speed;
    float hunger;

    Animal(Vector2 pos);
    void Update(float deltaTime) override;   // <-- override
    void Draw() const override;              // <-- override

    static Texture2D texture;
    static bool textureLoaded;

private:
    void Wander(float deltaTime);
};

#endif
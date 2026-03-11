#ifndef ANIMAL_H
#define ANIMAL_H
#include "raylib.h"

class Animal {
public:
    Vector2 position;
    Vector2 velocity;
    float speed;
    float hunger;

    Animal(Vector2 pos);
    void Update(float deltaTime);
    void Draw() const;
private:
    void Wander(float deltaTime);
};
#endif
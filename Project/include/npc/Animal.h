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
    void Update(float deltaTime, float worldW, float worldH);  // Добавлено
    void Draw() const;
    static Texture2D texture;
    static bool textureLoaded;
private:
    void Wander(float deltaTime, float worldW, float worldH);  // Добавлены параметры
};
#endif
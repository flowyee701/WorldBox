#ifndef ANIMAL_H
#define ANIMAL_H
#include "raylib.h"

class Terrain;

class Animal {
public:
    Vector2 position;
    Vector2 velocity;
    float speed;
    float hunger;

    Animal(Vector2 pos);
    void Update(float deltaTime, const Terrain* terrain);
    void Draw() const;
    static Texture2D texture;
    static bool textureLoaded;
private:
    void Wander(float deltaTime, const Terrain* terrain);
};
#endif
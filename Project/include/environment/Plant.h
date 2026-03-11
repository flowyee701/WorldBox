#ifndef PLANT_H
#define PLANT_H
#include "raylib.h"

class Plant {
public:
    Vector2 position;
    float growthStage;
    float health;
    Color color;

    Plant(Vector2 pos);
    void Update(float deltaTime);
    void Draw() const;
};
#endif
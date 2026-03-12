#ifndef PLANT_H
#define PLANT_H

#include "raylib.h"

enum class PlantType { FLOWER, TREE };

class Plant {
public:
    Vector2 position;
    float growthStage;
    float health;
    Color color;        // Оставим для совместимости или как фильтр
    PlantType type;     // Тип конкретного растения

    // --- СТАТИЧЕСКИЕ ТЕКСТУРЫ (общие для всех объектов класса) ---
    static Texture2D texFlower;
    static Texture2D texTree;
    static bool texturesLoaded;

    Plant(Vector2 pos);
    void Update(float deltaTime);
    void Draw() const;
};

#endif
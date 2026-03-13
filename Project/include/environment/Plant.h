// include/environment/Plant.h
#ifndef PLANT_H
#define PLANT_H

#include "raylib.h"
#include "WorldObject.h"   // <-- подключаем базовый класс (лежит в той же папке)

enum class PlantType { FLOWER, TREE };

class Plant : public WorldObject {   // <-- наследование
public:
    // constexpr константы
    static constexpr float GROWTH_RATE = 0.05f;
    static constexpr float BASE_FLOWER_SIZE = 24.0f;
    static constexpr float BASE_TREE_SIZE = 48.0f;

    Vector2 position;
    float growthStage;
    float health;
    Color color;
    PlantType type;

    static Texture2D texFlower;
    static Texture2D texTree;
    static bool texturesLoaded;

    Plant(Vector2 pos);
    void Update(float deltaTime) override;   // <-- override
    void Draw() const override;              // <-- override
};

#endif
// src/Plant.cpp
#include "environment/Plant.h"

Texture2D Plant::texFlower = { 0 };
Texture2D Plant::texTree = { 0 };
bool Plant::texturesLoaded = false;

Plant::Plant(Vector2 pos) : position(pos), growthStage(0.1f), health(100.0f) {
    type = (GetRandomValue(0, 1) == 0) ? PlantType::FLOWER : PlantType::TREE;
    color = (type == PlantType::TREE) ? DARKGREEN : RED;
}

void Plant::Update(float deltaTime) {
    if (growthStage < 1.0f) {
        growthStage += GROWTH_RATE * deltaTime;   // <-- используем константу
    }
}

void Plant::Draw() const {
    if (texturesLoaded) {
        Texture2D currentTex = (type == PlantType::FLOWER) ? texFlower : texTree;
        float baseSize = (type == PlantType::TREE) ? BASE_TREE_SIZE : BASE_FLOWER_SIZE; // <-- константы
        float finalSize = baseSize * growthStage;
        // ... остальная отрисовка без изменений
        Rectangle src = { 0.0f, 0.0f, (float)currentTex.width, (float)currentTex.height };
        Rectangle dst = { position.x, position.y, finalSize, finalSize };
        Vector2 origin = { finalSize / 2.0f, finalSize };
        DrawTexturePro(currentTex, src, dst, origin, 0.0f, WHITE);
    } else {
        DrawCircleV(position, 3.0f * growthStage, color);
    }
}
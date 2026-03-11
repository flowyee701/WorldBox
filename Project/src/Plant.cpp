#include "environment/Plant.h"

Plant::Plant(Vector2 pos) : position(pos), growthStage(0.1f), health(100.0f) {
    color = RED;
}

void Plant::Update(float deltaTime) {
    if (growthStage < 1.0f) {
        growthStage += 0.05f * deltaTime; // Медленный рост
    }
}

void Plant::Draw() const {
    float radius = 5.0f * growthStage;
    DrawCircleV(position, radius, color);
}
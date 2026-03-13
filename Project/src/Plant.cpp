#include "environment/Plant.h"

// 1. Инициализация статических переменных (обязательно вне функций)
Texture2D Plant::texFlower = { 0 };
Texture2D Plant::texTree = { 0 };
bool Plant::texturesLoaded = false;

// 2. Единственный правильный конструктор
Plant::Plant(Vector2 pos) : position(pos), growthStage(0.1f), health(100.0f) {
    // 50% шанс выбора типа при рождении, но тип передаётся извне
    type = PlantType::TREE; // По умолчанию дерево

    // Старый цвет оставляем для отрисовки прототипа (точки)
    color = (type == PlantType::TREE) ? DARKGREEN : RED;
}

// 2b. Конструктор с указанием типа
Plant::Plant(Vector2 pos, PlantType forcedType) : position(pos), growthStage(0.1f), health(100.0f) {
    type = forcedType;

    // Старый цвет оставляем для отрисовки прототипа (точки)
    color = (type == PlantType::TREE) ? DARKGREEN : RED;
}

// 3. Логика обновления
void Plant::Update(float deltaTime) {
    if (growthStage < 1.0f) {
        growthStage += 0.05f * deltaTime; // Растение постепенно растёт
    }
}

// 4. Метод отрисовки
void Plant::Draw() const {
    if (health <= 0.0f) return;
    
    if (texturesLoaded) {
        // Выбираем нужную текстуру в зависимости от типа
        Texture2D currentTex = (type == PlantType::FLOWER) ? texFlower : texTree;

        // Размер зависит от типа и стадии роста (growthStage)
        float baseSize = (type == PlantType::TREE) ? 48.0f : 24.0f;
        float finalSize = baseSize * growthStage;

        Rectangle src = { 0.0f, 0.0f, (float)currentTex.width, (float)currentTex.height };
        Rectangle dst = { position.x, position.y, finalSize, finalSize };

        // Центрируем по горизонтали и ставим на землю (origin.y = низ картинки)
        Vector2 origin = { finalSize / 2.0f, finalSize };

        DrawTexturePro(currentTex, src, dst, origin, 0.0f, WHITE);
    } else {
        // Если картинки ещё не загружены — рисуем кружок, размер которого растёт
        DrawCircleV(position, 3.0f * growthStage, color);
    }
}
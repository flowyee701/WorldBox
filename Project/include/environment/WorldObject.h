// include/environment/WorldObject.h
#ifndef WORLDOBJECT_H
#define WORLDOBJECT_H
class Terrain;

// Абстрактный базовый класс для всех объектов мира
class WorldObject {
public:
    virtual ~WorldObject() = default;

    // Чисто виртуальные методы – обязательны к реализации в наследниках
    virtual void Update(float deltaTime, const Terrain* terrain) = 0;
    virtual void Draw() const = 0;

protected:
    WorldObject() = default; // защищённый конструктор – нельзя создать экземпляр
};

#endif
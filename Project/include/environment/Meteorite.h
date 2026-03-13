#pragma once
#include <raylib.h>
#include "raymath.h"

class Meteorite {
public:
    Vector2 position;
    Vector2 velocity;
    Vector2 targetPosition;
    float radius;
    bool active;
    float rotation;
    float rotationSpeed;
    
    // Animation
    int currentFrame;
    float animationTimer;
    float animationSpeed;
    
    // Explosion
    bool exploding;
    float explosionTimer;
    float explosionRadius;
    float maxExplosionRadius;
    
    Meteorite();
    Meteorite(Vector2 startPos, Vector2 targetPos);
    
    void Update(float dt);
    void Draw(const Texture2D* textures = nullptr, const bool* loaded = nullptr) const;
    bool ShouldRemove() const;
    void StartExplosion();
    
    Rectangle GetCollisionRect() const;
    Vector2 GetCenter() const { return position; }
    
private:
    void UpdateExplosion(float dt);
    void UpdateAnimation(float dt);
};

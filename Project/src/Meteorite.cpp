#include "environment/Meteorite.h"
#include <cmath>

Meteorite::Meteorite() : 
    position{0, 0}, 
    velocity{0, 0},
    targetPosition{0, 0},
    radius(20.0f),
    active(false),
    rotation(0.0f),
    rotationSpeed(5.0f),
    currentFrame(0),
    animationTimer(0.0f),
    animationSpeed(0.08f),
    exploding(false),
    explosionTimer(0.0f),
    explosionRadius(0.0f),
    maxExplosionRadius(100.0f) {
}

Meteorite::Meteorite(Vector2 startPos, Vector2 targetPos) : 
    position(startPos),
    targetPosition(targetPos),
    radius(35.0f),
    active(true),
    rotation(0.0f),
    rotationSpeed(8.0f),
    currentFrame(0),
    animationTimer(0.0f),
    animationSpeed(0.08f),
    exploding(false),
    explosionTimer(0.0f),
    explosionRadius(0.0f),
    maxExplosionRadius(150.0f) {
    
    // Calculate velocity to reach target
    Vector2 direction = Vector2Subtract(targetPos, startPos);
    float distance = Vector2Length(direction);
    direction = Vector2Normalize(direction);
    
    // Speed based on distance for consistent travel time
    float speed = 300.0f;
    velocity = Vector2Scale(direction, speed);
    
    // Store target position for collision check
    targetPosition = targetPos;
}

void Meteorite::Update(float dt) {
    if (!active) return;
    
    if (exploding) {
        UpdateExplosion(dt);
    } else {
        // Update position
        position = Vector2Add(position, Vector2Scale(velocity, dt));
        
        // Update rotation
        rotation += rotationSpeed * dt;
        
        // Update animation
        UpdateAnimation(dt);
    }
}

void Meteorite::UpdateAnimation(float dt) {
    animationTimer += dt;
    if (animationTimer >= animationSpeed) {
        animationTimer -= animationSpeed;
        currentFrame = (currentFrame + 1) % 4; // Assuming 4 animation frames
    }
}

void Meteorite::UpdateExplosion(float dt) {
    explosionTimer += dt;
    explosionRadius = maxExplosionRadius * (explosionTimer / 0.5f); // 0.5 second explosion
    
    if (explosionTimer >= 0.5f) {
        active = false;
    }
}

void Meteorite::StartExplosion() {
    exploding = true;
    explosionTimer = 0.0f;
    explosionRadius = 0.0f;
}

void Meteorite::Draw(const Texture2D* textures, const bool* loaded) const {
    if (!active) return;
    
    if (exploding) {
        float alpha = 1.0f - (explosionTimer / 0.5f);
        Color explosionColor = {255, 100, 0, (unsigned char)(255 * alpha)};
        DrawCircleV(position, explosionRadius, explosionColor);
        
        Color innerExplosionColor = {255, 200, 0, (unsigned char)(200 * alpha)};
        DrawCircleV(position, explosionRadius * 0.6f, innerExplosionColor);
        
        Color coreExplosionColor = {255, 255, 200, (unsigned char)(180 * alpha)};
        DrawCircleV(position, explosionRadius * 0.3f, coreExplosionColor);
    } else {
        if (textures != nullptr && loaded != nullptr && loaded[currentFrame] && textures[currentFrame].id > 0) {
            float w = radius * 2.5f;
            float h = radius * 2.5f;
            Rectangle src{0, 0, (float)textures[currentFrame].width, (float)textures[currentFrame].height};
            Rectangle dst{
                floorf(position.x - w * 0.5f),
                floorf(position.y - h * 0.5f),
                w, h
            };
            DrawTexturePro(textures[currentFrame], src, dst, Vector2{0, 0}, rotation, WHITE);
        } else {
            Color meteoriteColor = {255, 120, 0, 255};
            DrawCircleV(position, radius, meteoriteColor);
            DrawCircleV(position, radius * 0.7f, {255, 200, 100, 255});
        }
    }
}

bool Meteorite::ShouldRemove() const {
    return !active;
}

Rectangle Meteorite::GetCollisionRect() const {
    return {
        position.x - radius,
        position.y - radius,
        radius * 2,
        radius * 2
    };
}

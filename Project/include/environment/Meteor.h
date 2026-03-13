#pragma once

#include "raylib.h"

class Meteor {
public:
    enum State { FALLING, EXPLODING, DONE };

    Vector2 pos;
    Vector2 targetPos;
    Vector2 velocity;
    float radius = 80.0f;
    float damage = 100.0f;
    State state = FALLING;
    float explosionTimer = 0.0f;
    float explosionDuration = 0.5f;
    float fallSpeed = 300.0f;

    Meteor(Vector2 target) {
        targetPos = target;
        pos = {target.x, target.y - 400.0f};
        velocity = {0.0f, fallSpeed};
    }

    void Update(float dt) {
        if (state == FALLING) {
            pos.y += velocity.y * dt;
            if (pos.y >= targetPos.y) {
                pos.y = targetPos.y;
                state = EXPLODING;
                explosionTimer = 0.0f;
            }
        } else if (state == EXPLODING) {
            explosionTimer += dt;
            if (explosionTimer >= explosionDuration) {
                state = DONE;
            }
        }
    }

    bool IsDone() const {
        return state == DONE;
    }
};

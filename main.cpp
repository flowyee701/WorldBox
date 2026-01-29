#include "raylib.h"
#include "raymath.h"
#include "world.h"

// простой clamp
inline float ClampFloat(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

int main() {
    InitWindow(1000, 700, "WorldBoxProto");
    SetTargetFPS(60);

    World world;
    world.worldW = 1400;
    world.worldH = 900;
    world.Init();

    Camera2D camera = {0};
    camera.target = { world.worldW / 2.0f, world.worldH / 2.0f };
    camera.offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    static Vector2 lastMouse = GetMousePosition();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        world.Update(dt);

        // ===============================
        // УПРАВЛЕНИЕ КАМЕРОЙ (ВАЖНО)
        // ===============================

        // --- перетаскивание ПКМ ---
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 mouse = GetMousePosition();
            Vector2 delta = Vector2Subtract(lastMouse, mouse);

            delta.x /= camera.zoom;
            delta.y /= camera.zoom;

            camera.target = Vector2Add(camera.target, delta);
        }

        lastMouse = GetMousePosition();

        // --- зум колёсиком ---
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            camera.zoom += wheel * 0.1f;
            camera.zoom = ClampFloat(camera.zoom, 0.4f, 2.5f);
        }

        // --- ограничение камеры ---
        camera.target.x = ClampFloat(camera.target.x, 0.0f, world.worldW);
        camera.target.y = ClampFloat(camera.target.y, 0.0f, world.worldH);

        // ===============================
        // РЕНДЕР
        // ===============================
        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(camera);
        world.Draw();
        EndMode2D();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
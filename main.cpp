#include "raylib.h"
#include "raymath.h"
#include "world.h"

enum class SpawnMode { NONE, CIVILIAN, WARRIOR };

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

    SpawnMode mode = SpawnMode::CIVILIAN;

    Vector2 lastMouse = GetMousePosition();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // ---- tool select ----
        if (IsKeyPressed(KEY_ONE)) mode = SpawnMode::CIVILIAN;
        if (IsKeyPressed(KEY_TWO)) mode = SpawnMode::WARRIOR;


        // ---- camera pan (RMB drag) ----
        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = Vector2Subtract(lastMouse, mouse);
            delta = Vector2Scale(delta, 1.0f / camera.zoom);
            camera.target = Vector2Add(camera.target, delta);
        }
        lastMouse = mouse;

        // ---- zoom ----
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            camera.zoom += wheel * 0.1f;
            camera.zoom = Clamp(camera.zoom, 0.4f, 2.5f);
        }

        // ---- spawn on LMB (only if not panning) ----
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);

            if (mouseWorld.x >= 0 && mouseWorld.x <= world.worldW &&
                mouseWorld.y >= 0 && mouseWorld.y <= world.worldH) {

                if (mode == SpawnMode::CIVILIAN) world.SpawnCivilian(mouseWorld);
                if (mode == SpawnMode::WARRIOR)  world.SpawnWarrior(mouseWorld);
            }
        }

        // ---- place bomb on RMB click (without dragging) ----
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
            if (mouseWorld.x >= 0 && mouseWorld.x <= world.worldW &&
                mouseWorld.y >= 0 && mouseWorld.y <= world.worldH) {
                world.AddBomb(mouseWorld.x, mouseWorld.y);
            }
        }

        world.Update(dt);

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(camera);
        world.Draw();
        EndMode2D();

        // UI hint
        DrawText(mode == SpawnMode::CIVILIAN ? "Mode: 1 CIVILIAN" :
                 mode == SpawnMode::WARRIOR  ? "Mode: 2 WARRIOR" : "Mode: NONE",
                 10, 10, 20, RAYWHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
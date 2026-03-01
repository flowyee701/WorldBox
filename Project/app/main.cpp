#include "raylib.h"
#include "raymath.h"
#include "environment/world.h"
#include <algorithm> // std::max

enum class SpawnMode { NONE, CIVILIAN, WARRIOR };
enum class WarriorRank { WARRIOR, CAPTAIN };

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1520, 1240, "WorldBoxProto");
    SetTargetFPS(60);

    World world;
    world.worldW = 1400;
    world.worldH = 900;
    world.Init();

    Camera2D camera = {0};
    camera.target = { world.worldW * 0.5f, world.worldH * 0.5f };
    camera.offset = { (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.5f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    SpawnMode mode = SpawnMode::CIVILIAN;
    WarriorRank warriorRank = WarriorRank::WARRIOR;

    Vector2 lastMouse = GetMousePosition();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        const int sw = GetScreenWidth();
        const int sh = GetScreenHeight();

        // --- always keep camera centered on screen ---
        camera.offset = { sw * 0.5f, sh * 0.5f };

        // --- "cover" zoom: карта всегда заполняет экран (без чёрных полос) ---
        float scaleX = (float)sw / (float)world.worldW;
        float scaleY = (float)sh / (float)world.worldH;
        camera.zoom = std::max(scaleX, scaleY);

        // ---- mode select ----
        if (IsKeyPressed(KEY_ONE)) {
            mode = SpawnMode::CIVILIAN;
        }

        if (IsKeyPressed(KEY_TWO)) {
            mode = SpawnMode::WARRIOR;
            warriorRank = WarriorRank::WARRIOR; // по умолчанию обычный воин
        }

        // В режиме WARRIOR: K -> капитан, W -> обычный воин
        if (mode == SpawnMode::WARRIOR) {
            if (IsKeyPressed(KEY_K)) warriorRank = WarriorRank::CAPTAIN;
            if (IsKeyPressed(KEY_W)) warriorRank = WarriorRank::WARRIOR;
        }

        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        // ---- camera pan (RMB drag) ----
        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = Vector2Subtract(lastMouse, mouse);
            delta = Vector2Scale(delta, 1.0f / camera.zoom);
            camera.target = Vector2Add(camera.target, delta);
        }
        lastMouse = mouse;

        // ---- OPTIONAL zoom by wheel (если хочешь оставить) ----
        // Сейчас у тебя cover-zoom всегда перетирает колесо.
        // Поэтому либо убираем колесо совсем, либо делаем множитель.
        // Оставлю как множитель:
        static float userZoom = 1.0f;
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            userZoom += wheel * 0.1f;
            userZoom = Clamp(userZoom, 0.6f, 2.0f);
        }
        camera.zoom *= userZoom;

        // ---- spawn on LMB (only if not panning) ----
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);

            // спавним только если внутри мира
            if (mouseWorld.x >= 0 && mouseWorld.x <= world.worldW &&
                mouseWorld.y >= 0 && mouseWorld.y <= world.worldH) {

                if (mode == SpawnMode::CIVILIAN) {
                    world.SpawnCivilian(mouseWorld);
                }
                else if (mode == SpawnMode::WARRIOR) {
                    if (warriorRank == WarriorRank::CAPTAIN) {
                        world.SpawnCaptain(mouseWorld);
                    } else {
                        world.SpawnWarrior(mouseWorld);
                    }
                }
            }
        }

        world.Update(dt);

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(camera);
        world.Draw();
        EndMode2D();

        // ---- UI ----
        const char* modeText = "Mode: NONE";
        if (mode == SpawnMode::CIVILIAN) modeText = "Mode: 1 CIVILIAN";
        if (mode == SpawnMode::WARRIOR)  modeText = "Mode: 2 WARRIOR";

        DrawText(modeText, 10, 10, 20, RAYWHITE);

        if (mode == SpawnMode::WARRIOR) {
            const char* rankText = (warriorRank == WarriorRank::CAPTAIN)
                                   ? "Rank: CAPTAIN (K)  back: W"
                                   : "Rank: WARRIOR (W)  captain: K";
            DrawText(rankText, 10, 35, 18, RAYWHITE);
        }

        EndDrawing();
    }

    world.Shutdown();
    CloseWindow();
    return 0;
}
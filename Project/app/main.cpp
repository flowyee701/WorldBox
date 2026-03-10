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
            // ---- captain auto/manual toggle ----
            if (IsKeyPressed(KEY_A)) {
                if (world.selectedCaptainIndex >= 0 && world.selectedCaptainIndex < (int)world.npcs.size()) {
                    NPC& cap = world.npcs[world.selectedCaptainIndex];
                    if (cap.alive && cap.humanRole == NPC::HumanRole::CAPTAIN) {
                        cap.captainAutoMode = !cap.captainAutoMode;

                        // если включили AUTO — снимаем ручной приказ (чтобы авто сразу работал)
                        if (cap.captainAutoMode) {
                            cap.captainHasMoveOrder = false;
                        }
                    }
                } else {
                    world.selectedCaptainIndex = -1;
                }
            }
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
        // ---- LMB actions ----
        // ---- LMB: Shift = captain control, иначе spawn ----
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);

            if (mouseWorld.x >= 0 && mouseWorld.x <= world.worldW &&
                mouseWorld.y >= 0 && mouseWorld.y <= world.worldH) {

                const bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

                if (shiftDown) {
                    // 1) попытка выбрать капитана кликом
                    const float pickR = 22.0f;
                    const float pickR2 = pickR * pickR;

                    int clickedCaptain = -1;
                    int clickedBandit = -1;

                    for (int i = 0; i < (int)world.npcs.size(); i++) {
                        const NPC& n = world.npcs[i];
                        if (!n.alive) continue;

                        float dx = n.pos.x - mouseWorld.x;
                        float dy = n.pos.y - mouseWorld.y;
                        float d2 = dx*dx + dy*dy;
                        if (d2 > pickR2) continue;

                        if (n.humanRole == NPC::HumanRole::CAPTAIN) {
                            clickedCaptain = i;
                            break; // приоритет капитану
                        }

                        if (n.humanRole == NPC::HumanRole::BANDIT) {
                            // не break: вдруг рядом капитан — он важнее
                            clickedBandit = i;
                        }
                    }


                    if (clickedCaptain != -1) {
                        world.selectedCaptainIndex = clickedCaptain;
                    }
                    else if (clickedBandit != -1) {
                        // ATTACK ORDER: если капитан выбран — атакуем группу бандита
                        if (world.selectedCaptainIndex >= 0 && world.selectedCaptainIndex < (int)world.npcs.size()) {
                            NPC& cap = world.npcs[world.selectedCaptainIndex];
                            if (cap.alive && cap.humanRole == NPC::HumanRole::CAPTAIN) {
                                const NPC& b = world.npcs[clickedBandit];

                                cap.captainAutoMode = false;   // приказ игрока
                                cap.captainHasMoveOrder = false;

                                cap.captainHasAttackOrder = true;
                                cap.captainAttackGroupId = b.banditGroupId;
                                cap.captainAttackTargetId = b.id;
                            }
                        } else {
                            world.selectedCaptainIndex = -1;
                        }
                    }
                    else {
                        // MOVE ORDER
                        if (world.selectedCaptainIndex >= 0 && world.selectedCaptainIndex < (int)world.npcs.size()) {
                            NPC& cap = world.npcs[world.selectedCaptainIndex];
                            if (cap.alive && cap.humanRole == NPC::HumanRole::CAPTAIN) {
                                cap.captainAutoMode = false;
                                cap.captainHasMoveOrder = true;
                                cap.captainMoveTarget = mouseWorld;
                            }
                        } else {
                            world.selectedCaptainIndex = -1;
                        }
                    }
                    // 1.5) если капитан не кликнут — проверим, не кликнули ли мы по бандиту
                    for (int i = 0; i < (int)world.npcs.size(); i++) {
                        const NPC& n = world.npcs[i];
                        if (!n.alive) continue;
                        if (n.humanRole != NPC::HumanRole::BANDIT) continue;

                        float dx = n.pos.x - mouseWorld.x;
                        float dy = n.pos.y - mouseWorld.y;
                        float d2 = dx*dx + dy*dy;
                        if (d2 <= pickR2) {
                            clickedBandit = i;
                            break;
                        }
                    }
                } else {
                    // обычный LMB = спавн (как было)
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
            DrawText("Shift+LMB: select/move captain | A: toggle AUTO/MANUAL", 10, 60, 18, RAYWHITE);
        }

        EndDrawing();
    }

    world.Shutdown();
    CloseWindow();
    return 0;
}
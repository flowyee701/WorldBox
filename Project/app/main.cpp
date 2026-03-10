#include "raylib.h"
#include "raymath.h"
#include "environment/world.h"
#include <algorithm>

enum class AppState
{
    MAP_MENU,
    GAME,
    PAUSED
};

enum class SpawnMode { CIVILIAN, WARRIOR };
enum class WarriorRank { WARRIOR, CAPTAIN };

static void ApplyBorderlessFullscreen() {
    int monitor = GetCurrentMonitor();
    Vector2 monitorPos = GetMonitorPosition(monitor);
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);

    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowPosition((int)monitorPos.x, (int)monitorPos.y);
    SetWindowSize(monitorWidth, monitorHeight);
}

static void ApplyWindowedMode(int width, int height) {
    ClearWindowState(FLAG_WINDOW_UNDECORATED);

    int monitor = GetCurrentMonitor();
    Vector2 monitorPos = GetMonitorPosition(monitor);
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);

    int x = (int)monitorPos.x + (monitorWidth - width) / 2;
    int y = (int)monitorPos.y + (monitorHeight - height) / 2;

    SetWindowSize(width, height);
    SetWindowPosition(x, y);
}

static int PickNpcIndexByRole(const World& world, Vector2 mouseWorld, NPC::HumanRole role, float radius) {
    const float r2 = radius * radius;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const NPC& n = world.npcs[i];
        if (!n.alive) continue;
        if (n.humanRole != role) continue;

        Vector2 pickPos = { n.pos.x, n.pos.y - 8.0f };

        float dx = pickPos.x - mouseWorld.x;
        float dy = pickPos.y - mouseWorld.y;
        float d2 = dx*dx + dy*dy;

        if (d2 <= r2) return i;
    }

    return -1;
}

static const char* GetCaptainModeLabel(const NPC& cap) {
    if (cap.captainHasAttackOrder) return "MANUAL ATTACK";
    if (!cap.captainAutoMode && cap.captainHasMoveOrder) return "MANUAL MOVE";
    if (!cap.captainAutoMode) return "MANUAL IDLE";
    return "AUTO";
}

int main() {
    const int windowedWidth = 1600;
    const int windowedHeight = 900;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
    InitWindow(windowedWidth, windowedHeight, "WorldBoxProto");
    SetTargetFPS(60);

    bool borderlessFullscreen = true;
    ApplyBorderlessFullscreen();

    AppState appState = AppState::MAP_MENU;

    World world;
    Camera2D camera = {0};

    float userZoom = 1.0f;
    SpawnMode mode = SpawnMode::CIVILIAN;
    WarriorRank warriorRank = WarriorRank::WARRIOR;
    Vector2 lastMouse = GetMousePosition();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_F5)) {
            borderlessFullscreen = !borderlessFullscreen;

            if (borderlessFullscreen) ApplyBorderlessFullscreen();
            else ApplyWindowedMode(windowedWidth, windowedHeight);
        }

        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        if (appState == AppState::MAP_MENU) {
            Vector2 mouse = GetMousePosition();
            bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

            float btnW = 260;
            float btnH = 50;
            Rectangle btnSmall = { (float)sw / 2 - btnW / 2, (float)sh / 2 - 100, btnW, btnH };
            Rectangle btnMedium = { (float)sw / 2 - btnW / 2, (float)sh / 2 - 30, btnW, btnH };
            Rectangle btnLarge = { (float)sw / 2 - btnW / 2, (float)sh / 2 + 40, btnW, btnH };

            int selectedW = 0;
            int selectedH = 0;

            if (IsKeyPressed(KEY_ONE) || (click && CheckCollisionPointRec(mouse, btnSmall))) {
                selectedW = 1400; selectedH = 900;
            } else if (IsKeyPressed(KEY_TWO) || (click && CheckCollisionPointRec(mouse, btnMedium))) {
                selectedW = 2200; selectedH = 1400;
            } else if (IsKeyPressed(KEY_THREE) || (click && CheckCollisionPointRec(mouse, btnLarge))) {
                selectedW = 3200; selectedH = 2000;
            }

            if (selectedW > 0) {
                world.worldW = selectedW;
                world.worldH = selectedH;
                world.Init();

                camera.target = { world.worldW * 0.5f, world.worldH * 0.5f };
                camera.offset = { (float)sw * 0.5f, (float)sh * 0.5f };
                camera.rotation = 0.0f;

                appState = AppState::GAME;
                lastMouse = GetMousePosition();
            }

            BeginDrawing();
            ClearBackground(BLACK);

            const char* title = "SELECT MAP SIZE";
            DrawText(title, sw / 2 - MeasureText(title, 30) / 2, sh / 2 - 180, 30, RAYWHITE);

            Color baseColor = { 40, 40, 40, 255 };
            Color hoverColor = { 70, 70, 70, 255 };
            Color outlineColor = { 100, 100, 100, 255 };

            bool hoverS = CheckCollisionPointRec(mouse, btnSmall);
            DrawRectangleRec(btnSmall, hoverS ? hoverColor : baseColor);
            DrawRectangleLinesEx(btnSmall, 2.0f, outlineColor);

            bool hoverM = CheckCollisionPointRec(mouse, btnMedium);
            DrawRectangleRec(btnMedium, hoverM ? hoverColor : baseColor);
            DrawRectangleLinesEx(btnMedium, 2.0f, outlineColor);

            bool hoverL = CheckCollisionPointRec(mouse, btnLarge);
            DrawRectangleRec(btnLarge, hoverL ? hoverColor : baseColor);
            DrawRectangleLinesEx(btnLarge, 2.0f, outlineColor);

            const char* txt1 = "1. SMALL (1400x900)";
            const char* txt2 = "2. MEDIUM (2200x1400)";
            const char* txt3 = "3. LARGE (3200x2000)";

            DrawText(txt1, btnSmall.x + btnW / 2 - MeasureText(txt1, 20) / 2, btnSmall.y + 15, 20, WHITE);
            DrawText(txt2, btnMedium.x + btnW / 2 - MeasureText(txt2, 20) / 2, btnMedium.y + 15, 20, WHITE);
            DrawText(txt3, btnLarge.x + btnW / 2 - MeasureText(txt3, 20) / 2, btnLarge.y + 15, 20, WHITE);

            EndDrawing();
        }
        else {
            if (appState == AppState::GAME) {
                if (world.selectedCaptainId != 0 && world.FindNpcById(world.selectedCaptainId) == nullptr) {
                    world.selectedCaptainId = 0;
                }

                camera.offset = { sw * 0.5f, sh * 0.5f };

                float fitZoom = std::max((float)sw / (float)world.worldW,
                                         (float)sh / (float)world.worldH);
                camera.zoom = fitZoom * userZoom;

                if (IsKeyPressed(KEY_ONE)) {
                    mode = SpawnMode::CIVILIAN;
                }

                if (IsKeyPressed(KEY_TWO)) {
                    mode = SpawnMode::WARRIOR;
                    warriorRank = WarriorRank::WARRIOR;
                }

                if (mode == SpawnMode::WARRIOR && IsKeyPressed(KEY_K)) {
                    warriorRank = (warriorRank == WarriorRank::WARRIOR)
                                  ? WarriorRank::CAPTAIN
                                  : WarriorRank::WARRIOR;
                }

                if (IsKeyPressed(KEY_A) && world.selectedCaptainId != 0) {
                    NPC* cap = world.FindNpcById(world.selectedCaptainId);
                    if (cap && cap->humanRole == NPC::HumanRole::CAPTAIN) {
                        cap->captainAutoMode = !cap->captainAutoMode;

                        if (cap->captainAutoMode) {
                            cap->captainHasMoveOrder = false;
                            cap->captainHasAttackOrder = false;
                            cap->captainAttackGroupId = -1;
                            cap->captainAttackTargetId = 0;
                        }
                    }
                }

                if (IsKeyPressed(KEY_ESCAPE)) {
                    if (world.selectedCaptainId != 0) {
                        world.selectedCaptainId = 0;
                    } else {
                        appState = AppState::PAUSED;
                    }
                }

                Vector2 mouse = GetMousePosition();
                if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                    Vector2 delta = Vector2Subtract(lastMouse, mouse);
                    delta = Vector2Scale(delta, 1.0f / camera.zoom);
                    camera.target = Vector2Add(camera.target, delta);
                }
                lastMouse = mouse;

                float wheel = GetMouseWheelMove();
                if (wheel != 0.0f) {
                    userZoom += wheel * 0.1f;
                    userZoom = Clamp(userZoom, 0.5f, 3.0f);
                }

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);

                    if (mouseWorld.x >= 0 && mouseWorld.x <= world.worldW &&
                        mouseWorld.y >= 0 && mouseWorld.y <= world.worldH) {

                        const bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

                        if (shiftDown) {
                            int clickedCaptain = PickNpcIndexByRole(world, mouseWorld, NPC::HumanRole::CAPTAIN, 18.0f);
                            int clickedBandit  = PickNpcIndexByRole(world, mouseWorld, NPC::HumanRole::BANDIT, 18.0f);

                            if (clickedCaptain != -1) {
                                world.selectedCaptainId = world.npcs[clickedCaptain].id;
                            }
                            else if (clickedBandit != -1 && world.selectedCaptainId != 0) {
                                NPC* cap = world.FindNpcById(world.selectedCaptainId);
                                if (cap && cap->humanRole == NPC::HumanRole::CAPTAIN) {
                                    const NPC& b = world.npcs[clickedBandit];

                                    cap->captainAutoMode = false;
                                    cap->captainHasMoveOrder = false;
                                    cap->captainMoveTarget = cap->pos;

                                    cap->captainHasAttackOrder = true;
                                    cap->captainAttackGroupId = b.banditGroupId;
                                    cap->captainAttackTargetId = b.id;
                                }
                            }
                            else if (world.selectedCaptainId != 0) {
                                world.IssueCaptainMoveOrder(world.selectedCaptainId, mouseWorld);
                            }
                        }
                        else {
                            if (mode == SpawnMode::CIVILIAN) {
                                world.SpawnCivilian(mouseWorld);
                            } else if (mode == SpawnMode::WARRIOR) {
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
            }
            else if (appState == AppState::PAUSED) {
                if (IsKeyPressed(KEY_ESCAPE)) {
                    appState = AppState::GAME;
                    lastMouse = GetMousePosition();
                }
            }

            BeginDrawing();
            ClearBackground(BLACK);

            BeginMode2D(camera);
            world.Draw();
            EndMode2D();

            int uiX = 20;
            int uiY = 20;
            int spacing = 26;

            const char* modeStr =
                    (mode == SpawnMode::CIVILIAN) ? "Mode: 1 CIVILIAN" :
                    (mode == SpawnMode::WARRIOR)  ? "Mode: 2 WARRIOR"  :
                    "Mode: ?";

            DrawText(modeStr, uiX, uiY, 20, RAYWHITE);
            uiY += spacing;

            if (mode == SpawnMode::WARRIOR) {
                const char* rankStr =
                        (warriorRank == WarriorRank::CAPTAIN)
                        ? "Rank: CAPTAIN (K)"
                        : "Rank: WARRIOR (K)";
                DrawText(rankStr, uiX, uiY, 20, RAYWHITE);
                uiY += spacing;
            }

            const char* t1="Shift+LMB Captain: Select";
            const char* t2="Shift+LMB Ground: Move selected captain";
            const char* t3="Shift+LMB Bandit: Attack whole bandit group";
            const char* t4="A: Toggle AUTO/MANUAL | Esc: Deselect/Pause | F5: Fullscreen";

            DrawText(t1, uiX, uiY, 20, RAYWHITE); uiY += spacing;
            DrawText(t2, uiX, uiY, 20, RAYWHITE); uiY += spacing;
            DrawText(t3, uiX, uiY, 20, RAYWHITE); uiY += spacing;
            DrawText(t4, uiX, uiY, 20, RAYWHITE); uiY += spacing;

            if (world.selectedCaptainId != 0) {
                NPC* cap = world.FindNpcById(world.selectedCaptainId);
                if (cap && cap->humanRole == NPC::HumanRole::CAPTAIN) {
                    uiY += 10;

                    const char* label = GetCaptainModeLabel(*cap);
                    const char* txt = TextFormat("Selected Captain: %s", label);

                    DrawText(txt, uiX, uiY, 20, YELLOW);
                    uiY += spacing;

                    int squadCount = 0;
                    for (const auto& n : world.npcs) {
                        if (!n.alive) continue;
                        if (n.humanRole != NPC::HumanRole::WARRIOR) continue;
                        if (n.leaderCaptainId == cap->id) squadCount++;
                    }

                    const char* squad = TextFormat("Squad: %d / 15", squadCount);
                    DrawText(squad, uiX, uiY, 20, YELLOW);
                }
            }

            if (appState == AppState::PAUSED) {
                DrawRectangle(0, 0, sw, sh, Color{ 0, 0, 0, 160 });
                const char* pauseMsg = "PAUSED";
                const char* resumeMsg = "Press ESC to resume";
                DrawText(pauseMsg, sw / 2 - MeasureText(pauseMsg, 50) / 2, sh / 2 - 40, 50, RAYWHITE);
                DrawText(resumeMsg, sw / 2 - MeasureText(resumeMsg, 20) / 2, sh / 2 + 30, 20, LIGHTGRAY);
            }

            EndDrawing();
        }
    }

    world.Shutdown();
    CloseWindow();
    return 0;
}
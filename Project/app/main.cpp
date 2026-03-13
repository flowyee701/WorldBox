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

enum class SpawnMode { CIVILIAN, WARRIOR, BUILD_BARRACKS };
enum class WarriorRank { WARRIOR, CAPTAIN };
enum class ToolMode { NONE, KILL, WAR };

// Applies a borderless fullscreen window layout
static void ApplyBorderlessFullscreen() {
    int monitor = GetCurrentMonitor();
    Vector2 monitorPos = GetMonitorPosition(monitor);
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);

    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowPosition((int)monitorPos.x, (int)monitorPos.y);
    SetWindowSize(monitorWidth, monitorHeight);
}

// Restores a centered windowed layout
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

// Picks the nearest alive NPC of a specific role near the cursor
static int PickNpcIndexByRole(const World& world, Vector2 mouseWorld, NPC::HumanRole role, float radius) {
    const float r2 = radius * radius;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const NPC& n = world.npcs[i];
        if (!n.alive || n.isDying) continue;
        if (n.humanRole != role) continue;

        Vector2 pickPos = { n.pos.x, n.pos.y - 8.0f };

        float dx = pickPos.x - mouseWorld.x;
        float dy = pickPos.y - mouseWorld.y;
        float d2 = dx*dx + dy*dy;

        if (d2 <= r2) return i;
    }

    return -1;
}

// Returns a short label for the selected captain mode
static const char* GetCaptainModeLabel(const NPC& cap) {
    if (cap.captainHasAttackOrder) return "MANUAL ATTACK";
    if (!cap.captainAutoMode && cap.captainHasMoveOrder) return "MANUAL MOVE";
    if (!cap.captainAutoMode) return "MANUAL IDLE";
    return "AUTO";
}

// Picks the nearest alive NPC regardless of role
static int PickAnyNpcIndex(const World& world, Vector2 mouseWorld, float radius) {
    const float r2 = radius * radius;
    int best = -1;
    float bestD2 = r2;

    for (int i = 0; i < (int)world.npcs.size(); i++) {
        const NPC& n = world.npcs[i];
        if (!n.alive || n.isDying) continue;

        Vector2 pickPos = { n.pos.x, n.pos.y - 8.0f };

        float dx = pickPos.x - mouseWorld.x;
        float dy = pickPos.y - mouseWorld.y;
        float d2 = dx*dx + dy*dy;

        if (d2 <= bestD2) {
            bestD2 = d2;
            best = i;
        }
    }

    return best;
}

// Finds the settlement under the cursor
static int PickSettlementIndexAtWorldPos(const World& world, Vector2 mouseWorld)
{
    for (int i = 0; i < (int)world.settlements.size(); i++) {
        const Settlement& s = world.settlements[i];
        if (!s.alive) continue;

        if (CheckCollisionPointRec(mouseWorld, s.boundsPx)) {
            return i;
        }
    }
    return -1;
}

// Resolves the current mode icon texture
static const Texture2D* GetCurrentModeIcon(const World& world,
                                           SpawnMode mode,
                                           WarriorRank warriorRank,
                                           bool toolsOpen,
                                           ToolMode toolMode,
                                           bool& loaded)
{
    loaded = false;

    if (toolsOpen && toolMode == ToolMode::KILL) {
        loaded = world.npcTexBanditLoaded[0];
        return &world.npcTexBandit[0];
    }

    if (toolsOpen && toolMode == ToolMode::WAR) {
        loaded = world.npcTexCaptainLoaded[0];
        return &world.npcTexCaptain[0];
    }

    if (!toolsOpen && mode == SpawnMode::BUILD_BARRACKS) {
        loaded = world.barracksTexLoaded;
        return &world.barracksTex;
    }

    if (mode == SpawnMode::CIVILIAN) {
        loaded = world.npcTexCivilianLoaded[0];
        return &world.npcTexCivilian[0];
    }

    if (warriorRank == WarriorRank::CAPTAIN) {
        loaded = world.npcTexCaptainLoaded[0];
        return &world.npcTexCaptain[0];
    }

    loaded = world.npcTexWarriorLoaded[0];
    return &world.npcTexWarrior[0];
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
    bool toolsOpen = false;
    ToolMode toolMode = ToolMode::NONE;
    int pendingWarSettlementA = -1;
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
                world.worldSeed = (unsigned int)GetRandomValue(1, 999999);
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

                if (IsKeyPressed(KEY_ZERO)) {
                    toolsOpen = !toolsOpen;
                    toolMode = toolsOpen ? ToolMode::KILL : ToolMode::NONE;
                    pendingWarSettlementA = -1;
                }

                if (!toolsOpen) {
                    if (IsKeyPressed(KEY_ONE)) {
                        mode = SpawnMode::CIVILIAN;
                    }

                    if (IsKeyPressed(KEY_TWO)) {
                        mode = SpawnMode::WARRIOR;
                        warriorRank = WarriorRank::WARRIOR;
                    }

                    if (IsKeyPressed(KEY_NINE)) {
                        mode = SpawnMode::BUILD_BARRACKS;
                    }

                    if (mode == SpawnMode::WARRIOR && IsKeyPressed(KEY_K)) {
                        warriorRank = (warriorRank == WarriorRank::WARRIOR)
                                      ? WarriorRank::CAPTAIN
                                      : WarriorRank::WARRIOR;
                    }
                } else {
                    if (IsKeyPressed(KEY_ONE)) {
                        toolMode = ToolMode::KILL;
                        pendingWarSettlementA = -1;
                    }
                    if (IsKeyPressed(KEY_TWO)) {
                        toolMode = ToolMode::WAR;
                        pendingWarSettlementA = -1;
                    }
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
                    userZoom = Clamp(userZoom, 0.5f, 12.0f);
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
                            if (toolsOpen && toolMode == ToolMode::KILL) {
                                int clickedNpc = PickAnyNpcIndex(world, mouseWorld, 18.0f);
                                if (clickedNpc != -1) {
                                    world.BeginNpcDeath(world.npcs[clickedNpc]);
                                }
                            }
                            else if (toolsOpen && toolMode == ToolMode::WAR) {
                                int clickedSettlement = PickSettlementIndexAtWorldPos(world, mouseWorld);
                                if (clickedSettlement != -1) {
                                    if (pendingWarSettlementA == -1) {
                                        pendingWarSettlementA = clickedSettlement;
                                    } else if (pendingWarSettlementA != clickedSettlement) {
                                        world.StartSettlementWar(pendingWarSettlementA, clickedSettlement);
                                        pendingWarSettlementA = -1;
                                    }
                                }
                            }
                            else if (mode == SpawnMode::BUILD_BARRACKS) {
                                world.TryBuildBarracksAt(mouseWorld);
                            }
                            else if (mode == SpawnMode::CIVILIAN) {
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

            if (!toolsOpen && mode == SpawnMode::BUILD_BARRACKS) {
                Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
                int tileX = (int)(mouseWorld.x / CELL_SIZE);
                int tileY = (int)(mouseWorld.y / CELL_SIZE);
                bool canBuild = false;
                Vector2 previewPos = {
                    (tileX + 0.5f) * CELL_SIZE,
                    (tileY + 0.5f) * CELL_SIZE
                };

                if (tileX >= 0 && tileX < world.cols && tileY >= 0 && tileY < world.rows) {
                    int tileId = tileY * world.cols + tileX;

                    for (const auto& s : world.settlements) {
                        if (!s.alive) continue;
                        if (!world.PointInSettlementPx(s, mouseWorld)) continue;

                        float dxFire = mouseWorld.x - s.campfirePosPx.x;
                        float dyFire = mouseWorld.y - s.campfirePosPx.y;
                        float fireDist2 = dxFire * dxFire + dyFire * dyFire;
                        if (fireDist2 < (CELL_SIZE * 5.0f) * (CELL_SIZE * 5.0f)) break;

                        if (s.tiles.find(tileId) == s.tiles.end()) break;

                        bool overlapsBarracks = false;
                        Vector2 snappedPos = {
                            (tileX + 0.5f) * CELL_SIZE,
                            (tileY + 0.5f) * CELL_SIZE
                        };
                        for (const auto& b : s.barracksList) {
                            if (!b.alive) continue;

                            float dx = snappedPos.x - b.posPx.x;
                            float dy = snappedPos.y - b.posPx.y;
                            float d2 = dx * dx + dy * dy;
                            if (d2 < (CELL_SIZE * 4.0f) * (CELL_SIZE * 4.0f)) {
                                overlapsBarracks = true;
                                break;
                            }
                        }
                        if (overlapsBarracks) break;

                        canBuild = true;
                        break;
                    }
                }

                Color previewColor = canBuild
                                     ? Color{255, 220, 120, 220}
                                     : Color{255, 100, 100, 220};
                float previewSize = CELL_SIZE * 1.6f;
                DrawRectangleLinesEx(
                    Rectangle{
                        floorf(previewPos.x - previewSize * 0.5f),
                        floorf(previewPos.y - previewSize * 0.5f),
                        previewSize,
                        previewSize
                    },
                    2.0f,
                    previewColor
                );
            }
            EndMode2D();

            int iconX = 20;
            int iconY = 20;
            int uiX = 64;
            int uiY = 20;
            int spacing = 26;

            bool modeIconLoaded = false;
            const Texture2D* modeIcon = GetCurrentModeIcon(world, mode, warriorRank, toolsOpen, toolMode, modeIconLoaded);

            DrawRectangle(iconX - 4, iconY - 4, 40, 40, Color{0, 0, 0, 120});
            DrawRectangleLines(iconX - 4, iconY - 4, 40, 40, Fade(RAYWHITE, 0.25f));

            if (modeIcon && modeIconLoaded && modeIcon->id != 0) {
                Rectangle src = {0.0f, 0.0f, (float)modeIcon->width, (float)modeIcon->height};
                Rectangle dst = {(float)iconX, (float)iconY, 32.0f, 32.0f};
                Color iconTint = (toolsOpen && toolMode == ToolMode::KILL)
                                 ? Color{255, 140, 140, 255}
                                 : WHITE;
                DrawTexturePro(*modeIcon, src, dst, Vector2{0,0}, 0.0f, iconTint);
            }

            const char* modeStr = toolsOpen
                    ? "Mode: 0 TOOLS"
                    : ((mode == SpawnMode::CIVILIAN) ? "Mode: 1 CIVILIAN"
                       : (mode == SpawnMode::WARRIOR) ? "Mode: 2 WARRIOR"
                                                      : "Mode: 9 BUILD BARRACKS");

            DrawText(modeStr, uiX, uiY, 20, RAYWHITE);
            uiY += spacing;

            if (toolsOpen) {
                const char* toolStr =
                        (toolMode == ToolMode::KILL) ? "Tool: 1 KILL NPC" :
                        (toolMode == ToolMode::WAR)  ? "Tool: 2 WAR" :
                                                       "Tool: NONE";
                Color toolColor =
                        (toolMode == ToolMode::KILL) ? Color{255, 170, 170, 255} :
                        (toolMode == ToolMode::WAR)  ? Color{255, 210, 120, 255} :
                                                       RAYWHITE;

                DrawText(toolStr, uiX, uiY, 20, toolColor);
                uiY += spacing;

                if (toolMode == ToolMode::WAR) {
                    if (pendingWarSettlementA == -1) {
                        DrawText("Select first settlement", uiX, uiY, 18, Color{255, 230, 180, 255});
                    } else {
                        DrawText("Select second settlement", uiX, uiY, 18, Color{255, 230, 180, 255});
                    }
                    uiY += spacing;
                }
            }
            else if (mode == SpawnMode::BUILD_BARRACKS) {
                DrawText("Build: click inside settlement", uiX, uiY, 20, Color{255, 220, 120, 255});
                uiY += spacing;
                DrawText("Needs free spot away from campfire", uiX, uiY, 18, Color{255, 220, 120, 255});
                uiY += spacing;
            }
            else if (mode == SpawnMode::WARRIOR) {
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
            const char* t4="0: Tools | Tools: 1 Kill, 2 War | 9: Build Barracks | A: AUTO/MANUAL | Esc: Deselect/Pause | F5: Fullscreen";

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

#include "environment/world.h"
#include "npc/human_behavior.h"
#include "npc/bandit_behavior.h"
#include "environment/settlement.h"
#include "npc/npc.h"
#include <algorithm>
#include <cfloat>
#include <iostream>
#include <string>
// ------------------------------------------------------------
// Helpers
static Vector2 RandomOutsideSpawn(int w, int h) {
    const float margin = 80.0f;

    int side = GetRandomValue(0, 3);
    switch (side) {
        case 0:
            return {-margin, (float) GetRandomValue(0, h)};          // left
        case 1:
            return {(float) w + margin, (float) GetRandomValue(0, h)};// right
        case 2:
            return {(float) GetRandomValue(0, w), -margin};          // top
        default:
            return {(float) GetRandomValue(0, w), (float) h + margin};// bottom
    }
}
static float ClampF(float v, float a, float b) { return (v < a) ? a : (v > b) ? b : v; }
static Vector2 NearestTileCenterPx(const World& world, const Settlement& s, Vector2 from)
{
    if (s.tiles.empty()) return from;

    float bestD2 = 1e30f;
    Vector2 best = from;

    for (int tile : s.tiles) {
        int cx = tile % world.cols;
        int cy = tile / world.cols;

        Vector2 c{
                (cx + 0.5f) * CELL_SIZE,
                (cy + 0.5f) * CELL_SIZE
        };

        float dx = c.x - from.x;
        float dy = c.y - from.y;
        float d2 = dx*dx + dy*dy;

        if (d2 < bestD2) { bestD2 = d2; best = c; }
    }
    return best;
}
static Vector2 ClosestPointInRectPx(Vector2 p, const Rectangle& rPx)
{
    return {
            ClampF(p.x, rPx.x, rPx.x + rPx.width),
            ClampF(p.y, rPx.y, rPx.y + rPx.height)
    };
}
static std::string PathJoin(const char* a, const char* b) {
    std::string s = a;
    if (!s.empty() && s.back() != '/') s += "/";
    s += b;
    return s;
}

static bool RectsOverlap(const Rectangle &a, const Rectangle &b) {
    return (a.x < b.x + b.width) && (a.x + a.width > b.x) &&
           (a.y < b.y + b.height) && (a.y + a.height > b.y);
}

static Rectangle ZoneToPxRect(const Rectangle &z) {
    return Rectangle{
            z.x * CELL_SIZE,
            z.y * CELL_SIZE,
            z.width * CELL_SIZE,
            z.height * CELL_SIZE
    };
}


// Ищем файл в нескольких местах относительно Working Directory
static std::string FindAssetPath(const char* relativePath)
{
    const char* wd = GetWorkingDirectory();

    // чаще всего у тебя WD = .../cmake-build-debug/Project/app
    // а assets лежит в .../Project/assets
    // значит рабочий путь: "../assets/...."
    const char* candidates[] = {
            "",         // wd/relativePath
            "../",      // wd/../relativePath
            "../../",   // wd/../../relativePath
            "../../../" // на всякий случай
    };

    for (const char* base : candidates) {
        std::string full = PathJoin(wd, (std::string(base) + relativePath).c_str());
        if (FileExists(full.c_str())) return full;
    }

    return "";
}

void World::LoadNpcSprites()
{

    if (npcSpritesLoaded) return;

    auto loadOne = [&](Texture2D& outTex, bool& outLoaded, const char* relPath) {
        std::string p = FindAssetPath(relPath);
        if (p.empty()) {
            TraceLog(LOG_ERROR, "NPC SPRITE missing: %s (WD=%s)", relPath, GetWorkingDirectory());
            outLoaded = false;
            return;
        }

        outTex = LoadTexture(p.c_str());
        if (outTex.id == 0) {
            TraceLog(LOG_ERROR, "Failed to LoadTexture: %s", p.c_str());
            outLoaded = false;
            return;
        }

        SetTextureFilter(outTex, TEXTURE_FILTER_POINT);
        SetTextureWrap(outTex, TEXTURE_WRAP_CLAMP);
        outLoaded = true;

        TraceLog(LOG_INFO, "Loaded sprite: %s (%dx%d)", p.c_str(), outTex.width, outTex.height);
    };

    for (int i = 0; i < NPC_VARIANTS; i++) {
        loadOne(npcTexCivilian[i], npcTexCivilianLoaded[i],
                (std::string("assets/npc/civilian/civilian_") + std::to_string(i) + ".png").c_str());

        loadOne(npcTexWarrior[i], npcTexWarriorLoaded[i],
                (std::string("assets/npc/warrior/warrior_") + std::to_string(i) + ".png").c_str());

        loadOne(npcTexBandit[i], npcTexBanditLoaded[i],
                (std::string("assets/npc/bandit/bandit_") + std::to_string(i) + ".png").c_str());
    }
    loadOne(npcTexCaptain, npcTexCaptainLoaded, "assets/npc/captain/captain_0.png");

    npcSpritesLoaded = true;
}

void World::UnloadNpcSprites()
{
    for (int i = 0; i < NPC_VARIANTS; i++) {
        if (npcTexCivilianLoaded[i]) { UnloadTexture(npcTexCivilian[i]); npcTexCivilianLoaded[i] = false; }
        if (npcTexWarriorLoaded[i])  { UnloadTexture(npcTexWarrior[i]);  npcTexWarriorLoaded[i]  = false; }
        if (npcTexBanditLoaded[i])   { UnloadTexture(npcTexBandit[i]);   npcTexBanditLoaded[i]   = false; }
        if (npcTexCaptainLoaded) { UnloadTexture(npcTexCaptain); npcTexCaptainLoaded = false; }
    }
    npcSpritesLoaded = false;
}

void World::LoadFireSprites()
{
    auto loadOne = [&](Texture2D& outTex, bool& outLoaded, const char* relPath) {
        std::string p = FindAssetPath(relPath); // у тебя FindAssetPath уже есть
        if (p.empty()) {
            TraceLog(LOG_ERROR, "FIRE missing: %s (WD=%s)", relPath, GetWorkingDirectory());
            outLoaded = false;
            return;
        }
        outTex = LoadTexture(p.c_str());
        if (outTex.id == 0) {
            TraceLog(LOG_ERROR, "Failed to LoadTexture fire: %s", p.c_str());
            outLoaded = false;
            return;
        }
        SetTextureFilter(outTex, TEXTURE_FILTER_POINT);
        SetTextureWrap(outTex, TEXTURE_WRAP_CLAMP);
        outLoaded = true;
        TraceLog(LOG_INFO, "Loaded fire: %s", p.c_str());
    };

    for (int i = 0; i < FIRE_FRAMES; i++) {
        std::string rel = "assets/environment/fire/fire_" + std::to_string(i) + ".png";
        loadOne(fireTex[i], fireLoaded[i], rel.c_str());
    }
}

void World::UnloadFireSprites()
{
    for (int i = 0; i < FIRE_FRAMES; i++) {
        if (fireLoaded[i]) {
            UnloadTexture(fireTex[i]);
            fireLoaded[i] = false;
        }
    }
}

void World::UpdateCampfires()
{
    for (auto& s : settlements) {
        if (!s.alive) continue;

        Vector2 c = s.centerPx;

        // если вдруг центр не внутри — ставим в ближайший тайл
        if (!PointInSettlementPx(s, c)) {
            c = NearestTileCenterPx(*this, s, c);
        }

        s.campfirePosPx = c;
    }
}

bool World::PointInSettlementPx(const Settlement& s, Vector2 pos) const {
    int cx = (int)(pos.x / CELL_SIZE);
    int cy = (int)(pos.y / CELL_SIZE);

    if (cx < 0 || cy < 0 || cx >= cols || cy >= rows)
        return false;

    int tileId = cy * cols + cx;
    return s.tiles.find(tileId) != s.tiles.end();
}

Vector2 World::ComputeSettlementCenterPx(const Settlement& s) {
    if (s.tiles.empty()) return {0.0f, 0.0f};

    double sx = 0.0, sy = 0.0;
    int n = 0;

    for (int tile : s.tiles) {
        int cx = tile % cols;
        int cy = tile / cols;

        sx += (cx + 0.5) * CELL_SIZE;
        sy += (cy + 0.5) * CELL_SIZE;
        n++;
    }

    return { (float)(sx / n), (float)(sy / n) };
}

Rectangle World::ComputeSettlementBoundsPx(const Settlement& s) {
    if (s.tiles.empty()) return {0,0,0,0};

    int minX = cols, minY = rows;
    int maxX = -1, maxY = -1;

    for (int tile : s.tiles) {
        int cx = tile % cols;
        int cy = tile / cols;

        if (cx < minX) minX = cx;
        if (cy < minY) minY = cy;
        if (cx > maxX) maxX = cx;
        if (cy > maxY) maxY = cy;
    }

    return {
            (float)(minX * CELL_SIZE),
            (float)(minY * CELL_SIZE),
            (float)((maxX - minX + 1) * CELL_SIZE),
            (float)((maxY - minY + 1) * CELL_SIZE)
    };
}


// ------------------------------------------------------------
void World::MergeSettlementsIfNeeded() {
    for (int i = 0; i < (int)settlements.size(); i++) {
        if (!settlements[i].alive) continue;

        for (int j = i + 1; j < (int)settlements.size(); j++) {
            if (!settlements[j].alive) continue;

            bool touching = false;

            for (int tile : settlements[j].tiles) {
                if (settlements[i].tiles.count(tile)) {
                    touching = true;
                    break;
                }
            }

            if (!touching) continue;

            // объединяем j -> i
            for (int tile : settlements[j].tiles)
                settlements[i].tiles.insert(tile);

            for (auto& npc : npcs)
                if (npc.settlementId == j)
                    npc.settlementId = i;

            settlements[j].tiles.clear();
            settlements[j].alive = false;

            settlements[i].centerPx = ComputeSettlementCenterPx(settlements[i]);
            settlements[i].boundsPx = ComputeSettlementBoundsPx(settlements[i]);
        }
    }
}

static Vector2 RandomEdgeSpawn(int w, int h) {
    int side = GetRandomValue(0, 3);
    switch (side) {
        case 0:
            return {0.0f, (float) GetRandomValue(0, h)};        // left
        case 1:
            return {(float) w, (float) GetRandomValue(0, h)};    // right
        case 2:
            return {(float) GetRandomValue(0, w), 0.0f};        // top
        default:
            return {(float) GetRandomValue(0, w), (float) h};   // bottom
    }
}

void World::SpawnCivilian(Vector2 pos) {

    NPC npc;
    npc.type = NPC::Type::HUMAN;
    npc.humanRole = NPC::HumanRole::CIVILIAN;
    npc.skinId = (uint16_t)GetRandomValue(0, 2); // 0..2
    npc.pos = pos;
    npc.vel = {0, 0};
    npc.speed = 15.0f;
    npc.hp = 100.0f;
    npc.alive = true;
    npc.settlementId = -1;
    npc.damage = 0.0f;

    // 1) если клик внутри существующего поселения — просто присоединяем
    int sid = -1;
    for (int i = 0; i < (int) settlements.size(); i++) {
        if (!settlements[i].alive) continue;
        if (PointInSettlementPx(settlements[i], pos)) {
            sid = i;
            break;
        }
    }
    if (sid != -1) {
        npc.settlementId = sid;
        npcs.push_back(npc);
        return;
    }

    // 2) иначе — ищем рядом "свободных" жителей без поселения
    if (sid == -1) {
        std::vector<int> nearbyFreeCivs;
        for (int i = 0; i < (int) npcs.size(); i++) {
            const auto &o = npcs[i];
            if (!o.alive) continue;
            if (o.humanRole != NPC::HumanRole::CIVILIAN) continue;
            if (o.settlementId != -1) continue;

            float dx = o.pos.x - pos.x;
            float dy = o.pos.y - pos.y;
            if (dx * dx + dy * dy < 90.0f * 90.0f) nearbyFreeCivs.push_back(i);
        }

        // если есть 2 рядом — создаём новое поселение (3-й будет текущий npc)
        if (nearbyFreeCivs.size() >= 2) {
            Settlement s;
            s.alive = true;

            int cx = (int)(pos.x / CELL_SIZE);
            int cy = (int)(pos.y / CELL_SIZE);

            constexpr int R = 8; // радиус поселения в клетках

            for (int dy = -R; dy <= R; dy++) {
                for (int dx = -R; dx <= R; dx++) {
                    int tx = cx + dx;
                    int ty = cy + dy;
                    if (tx < 0 || ty < 0 || tx >= cols || ty >= rows) continue;
                    s.tiles.insert(ty * cols + tx);
                }
            }

            s.color = Color{
                    (unsigned char)GetRandomValue(80,255),
                    (unsigned char)GetRandomValue(80,255),
                    (unsigned char)GetRandomValue(80,255),
                    255
            };

            settlements.push_back(s);
            int sid = (int)settlements.size() - 1;

            settlements[sid].centerPx = ComputeSettlementCenterPx(settlements[sid]);
            settlements[sid].boundsPx = ComputeSettlementBoundsPx(settlements[sid]);

            npc.settlementId = sid;
            npcs[nearbyFreeCivs[0]].settlementId = sid;
            npcs[nearbyFreeCivs[1]].settlementId = sid;
        }
        npcs.push_back(npc);
    }
}

void World::SpawnWarrior(Vector2 pos) {
    NPC npc;
    npc.type = NPC::Type::HUMAN;
    npc.humanRole = NPC::HumanRole::WARRIOR;
    npc.skinId = (uint16_t)GetRandomValue(0, 2);
    npc.pos = pos;
    npc.vel = {0,0};
    npc.hp = 200.0f;
    npc.damage = 20.0f;
    npc.settlementId = -1;

    // 🔹 НОВОЕ: если клик внутри поселения — сразу привязываем
    for (int i = 0; i < (int)settlements.size(); i++) {
        if (!settlements[i].alive) continue;
        if (PointInSettlementPx(settlements[i], pos)) {
            npc.settlementId = i;
            break;
        }
    }

    npcs.push_back(npc);
}
void World::SpawnCaptain(Vector2 pos) {
    NPC npc;
    npc.type = NPC::Type::HUMAN;
    npc.humanRole = NPC::HumanRole::CAPTAIN; // <-- ВАЖНО
    npc.isCaptain = true;

    npc.skinId = 0; // если позже сделаешь варианты капитана
    npc.pos = pos;
    npc.vel = {0,0};

    npc.hp = 350.0f;
    npc.damage = 35.0f;
    npc.speed = 35.0f;

    npc.settlementId = -1;

    // если клик внутри поселения — привязываем
    for (int i = 0; i < (int)settlements.size(); i++) {
        if (!settlements[i].alive) continue;
        if (PointInSettlementPx(settlements[i], pos)) {
            npc.settlementId = i;
            break;
        }
    }

    npcs.push_back(npc);
}

static void DrawBanditTriangle(Vector2 pos, float size, Color color) {
    Vector2 p1 = {pos.x, pos.y + size};
    Vector2 p2 = {pos.x + size, pos.y - size};
    Vector2 p3 = {pos.x - size, pos.y - size};
    DrawTriangle(p1, p2, p3, color);
    DrawTriangleLines(p1, p2, p3, BLACK);
}

// ------------------------------------------------------------
// Init
// ------------------------------------------------------------

void World::Init()
{
    cols = worldW / CELL_SIZE;
    rows = worldH / CELL_SIZE;

    settlements.clear();
    npcs.clear();

    banditSpawnTimer = 0.0f;
    nextBanditGroupId = 1;

    LoadNpcSprites(); // <-- ВАЖНО

    LoadFireSprites();
    UpdateCampfires();
}
void World::Shutdown()
{
    UnloadNpcSprites();
    UnloadFireSprites();
}



// ------------------------------------------------------------
// Update
// ------------------------------------------------------------

void World::Update(float dt) {

    // -------- bandit group spawning (DEBUG: often) --------
    banditSpawnTimer -= dt;


    if (banditSpawnTimer <= 0.0f) {
        banditSpawnTimer = 45.0f; // frequent for debug

        int count = GetRandomValue(5, 8);
        Vector2 spawnPos = RandomOutsideSpawn(worldW, worldH);

// направление ВСЕГДА внутрь карты
        Vector2 toWorldCenter = {
                worldW * 0.5f - spawnPos.x,
                worldH * 0.5f - spawnPos.y
        };
        Vector2 dir = SafeNormalize(toWorldCenter);
        int gid = nextBanditGroupId++;

        for (int i = 0; i < count; i++) {
            NPC npc;
            npc.type = NPC::Type::HUMAN;
            npc.humanRole = NPC::HumanRole::BANDIT;
            npc.skinId = (uint16_t)GetRandomValue(0, 2);
            npc.settlementId = -1;

            npc.banditGroupId = gid;
            npc.banditGroupDir = dir;

            npc.speed = 40.0f;
            npc.pos = {
                    spawnPos.x + GetRandomValue(-10, 10),
                    spawnPos.y + GetRandomValue(-10, 10)
            };
            npc.vel = {dir.x * npc.speed, dir.y * npc.speed};

            npcs.push_back(npc);
        }
    }

    // -------- update all NPCs --------
    for (auto &npc: npcs) {
        if (npc.type == NPC::Type::HUMAN) {
            HumanBehavior::Update(*this, npc, dt);
        }
    }
    // анимация костра
    fireAnimT += dt;
    if (fireAnimT >= fireAnimSpeed) {
        fireAnimT = 0.0f;
        fireFrame = (fireFrame + 1) % FIRE_FRAMES;
    }

// если поселения меняются/мерджатся — пересчитываем позиции костров
    UpdateCampfires();
    // --- автопривязка диких NPC к первому поселению, в которое они вошли ---
    for (auto& npc : npcs) {
        if (!npc.alive) continue;
        if (npc.settlementId != -1) continue;

        if (npc.humanRole != NPC::HumanRole::CIVILIAN &&
            npc.humanRole != NPC::HumanRole::WARRIOR) {
            continue;
        }

        for (int i = 0; i < (int)settlements.size(); i++) {
            if (!settlements[i].alive) continue;
            if (PointInSettlementPx(settlements[i], npc.pos)) {
                npc.settlementId = i;
                break;
            }
        }
    }

    // -------- cleanup bandits outside map --------
    npcs.erase(
            std::remove_if(npcs.begin(), npcs.end(),
                           [&](const NPC &n) {
                               return n.humanRole == NPC::HumanRole::BANDIT &&
                                      (n.pos.x < -100 || n.pos.y < -100 ||
                                       n.pos.x > worldW + 100 || n.pos.y > worldH + 100);
                           }),
            npcs.end()
    );
    npcs.erase(
            std::remove_if(npcs.begin(), npcs.end(),
                           [](const NPC &n) { return !n.alive; }),
            npcs.end()
    );


    for (auto &s: settlements) {
        if (!s.alive) continue;

        bool anyoneLeft = false;
        for (const auto &npc: npcs) {
            if (!npc.alive) continue;
            if (npc.settlementId == (&s - &settlements[0]) &&
                npc.humanRole != NPC::HumanRole::BANDIT) {
                anyoneLeft = true;
                break;
            }
        }

        if (!anyoneLeft) {
            s.alive = false;
        }
    }

    MergeSettlementsIfNeeded();
}

// ------------------------------------------------------------
// Draw
// ------------------------------------------------------------
Color GetSafeSettlementColor(const World &w, int sid) {
    Color c = {220, 220, 220, 255};

    if (sid >= 0 && sid < (int)w.settlements.size() && w.settlements[sid].alive) {
        c = w.settlements[sid].color;
    }

    c.a = 255; // <- ЖЕЛЕЗНО непрозрачный всегда
    return c;
}

static void DrawDiamondSolid(Vector2 center, int r, Color col)
{
    // col.a игнорируем не будем — но на всякий случай
    col.a = 255;

    int cx = (int)center.x;
    int cy = (int)center.y;

    // верхняя половина (включая середину)
    for (int dy = -r; dy <= 0; dy++) {
        int half = r + dy; // 0..r
        DrawLine(cx - half, cy + dy, cx + half, cy + dy, col);
    }

    // нижняя половина
    for (int dy = 1; dy <= r; dy++) {
        int half = r - dy; // r-1..0
        DrawLine(cx - half, cy + dy, cx + half, cy + dy, col);
    }
}

static void DrawDiamondOutline(Vector2 center, int r, Color col)
{
    col.a = 255;
    int cx = (int)center.x;
    int cy = (int)center.y;

    Vector2 top    = {(float)cx,     (float)(cy - r)};
    Vector2 right  = {(float)(cx+r), (float)cy};
    Vector2 bottom = {(float)cx,     (float)(cy + r)};
    Vector2 left   = {(float)(cx-r), (float)cy};

    DrawLineV(top, right, col);
    DrawLineV(right, bottom, col);
    DrawLineV(bottom, left, col);
    DrawLineV(left, top, col);
}

void World::Draw() const {
    // grass
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            Color base = ((x + y) % 2 == 0)
                         ? Color{60, 120, 60, 255}
                         : Color{55, 110, 55, 255};
            DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, base);
        }
    }

    // settlements
    for (const auto& s : settlements) {
        if (!s.alive) continue;

        Color fill = Fade(s.color, 0.25f);

        for (int tile : s.tiles) {
            int cx = tile % cols;
            int cy = tile / cols;

            DrawRectangle(
                    cx * CELL_SIZE,
                    cy * CELL_SIZE,
                    CELL_SIZE,
                    CELL_SIZE,
                    fill
            );
        }
    }


    // =====================
// NPCs (fixed size by CELL_SIZE)
// =====================
    for (const auto& npc : npcs) {

        if (!npc.alive) continue;

        int v = (int)(npc.skinId % NPC_VARIANTS);

        const Texture2D* tex = nullptr;
        bool loaded = false;


        switch (npc.humanRole) {
            case NPC::HumanRole::CIVILIAN:
                tex = &npcTexCivilian[v];
                loaded = npcTexCivilianLoaded[v];
                break;
            case NPC::HumanRole::WARRIOR:
                tex = &npcTexWarrior[v];
                loaded = npcTexWarriorLoaded[v];
                break;
            case NPC::HumanRole::BANDIT:
                tex = &npcTexBandit[v];
                loaded = npcTexBanditLoaded[v];
                break;
            case NPC::HumanRole::CAPTAIN:
                tex = &npcTexCaptain;
                loaded = npcTexCaptainLoaded;
                break;
            default:
                break;
        }
        float mult = 1.0f; // общий множитель
        if (npc.humanRole == NPC::HumanRole::CIVILIAN) mult = 1.15f; // жители чуть больше
        if (npc.humanRole == NPC::HumanRole::BANDIT)  mult = 1.05f; // чуть крупнее (или 1.0f)

        // базовый размер на карте: 2 клетки (как ты хотел раньше)
        float w = (float)CELL_SIZE * 2.0f * mult;
        float h = (float)CELL_SIZE * 2.0f * mult;

        Rectangle src{ 0.0f, 0.0f, (float)tex->width, (float)tex->height };

        // якорим "ноги" в npc.pos: низ спрайта = npc.pos.y
        Rectangle dst{
                floorf(npc.pos.x - w * 0.5f),
                floorf(npc.pos.y - h * 0.5f),
                w, h
        };

        // если спрайта нет — fallback
        if (!tex || !loaded || tex->id == 0) {
            float size = CELL_SIZE * 0.4f;
            Color c = GetSafeSettlementColor(*this, npc.settlementId);

            switch (npc.humanRole) {
                case NPC::HumanRole::CIVILIAN: DrawCircleV(npc.pos, size, c); break;
                case NPC::HumanRole::WARRIOR:  {
                    DrawRectangle(npc.pos.x-size, npc.pos.y-size, size*2, size*2, c); break;
                }
                case NPC::HumanRole::BANDIT:   DrawTriangle(
                            {npc.pos.x, npc.pos.y + CELL_SIZE*0.7f},
                            {npc.pos.x + CELL_SIZE*0.7f, npc.pos.y - CELL_SIZE*0.7f},
                            {npc.pos.x - CELL_SIZE*0.7f, npc.pos.y - CELL_SIZE*0.7f},
                            Color{160,80,200,255}); break;
                default: break;
            }
            continue;
        }

        // --- ВСЕГДА рисуем в размере от клетки, а не от tex->width ---


        // НЕ ТИНТУЕМ спрайт (иначе можно получить "пропал" из-за альфы/смешивания)
        DrawTexturePro(*tex, src, dst, Vector2{0,0}, 0.0f, WHITE);

        // --- обозначение поселения: плотный пиксельный ромбик над головой ---
        if (npc.settlementId != -1) {
            Color sc = GetSafeSettlementColor(*this, npc.settlementId); // уже a=255
            sc.a = 255;

            // позиция над головой: top спрайта = npc.pos.y - h
            Vector2 c = {
                    (float)((int)npc.pos.x),
                    (float)((int)(npc.pos.y - h - 6.0f))  // чуть выше; подстрой 4..8
            };

            const int r = 3; // размер ромбика
            DrawDiamondSolid(c, r, sc);
            DrawDiamondOutline(c, r, BLACK);
        }
    }
    // ---- CAMPFIRES ----
    int f = fireFrame;
    if (f < 0 || f >= FIRE_FRAMES) f = 0;

    for (const auto& s : settlements) {
        if (!s.alive) continue;
        if (!fireLoaded[f] || fireTex[f].id == 0) continue;

        // рисуем крупнее, чем NPC
        float w = (float)CELL_SIZE * 4.0f;
        float h = (float)CELL_SIZE * 4.0f;

        Rectangle src{0,0,(float)fireTex[f].width,(float)fireTex[f].height};
        Rectangle dst{
                floorf(s.campfirePosPx.x - w*0.5f),
                floorf(s.campfirePosPx.y - h*0.5f),
                w, h
        };


        DrawTexturePro(fireTex[f], src, dst, {0,0}, 0.0f, WHITE);
    }
}
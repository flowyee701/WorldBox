#include "terrain/terrain.h"
#include <cmath>
#include <algorithm>

Terrain::Terrain(int width, int height, unsigned int seed)
    : width(width), height(height), seed(seed),
      tiles(width * height) {

    TileProperties deepWaterProps;
    deepWaterProps.moveSpeed = 0.0f;
    deepWaterProps.canWalk  = false;
    deepWaterProps.canSwim  = true;
    deepWaterProps.canBuild = false;
    deepWaterProps.isWater  = true;
    deepWaterProps.tint     = {5, 20, 80, 255};


    TileProperties shallowWaterProps;
    shallowWaterProps.moveSpeed = 0.0f;
    shallowWaterProps.canWalk  = false;
    shallowWaterProps.canSwim  = true;
    shallowWaterProps.canBuild = false;
    shallowWaterProps.isWater  = true;
    shallowWaterProps.tint     = {25, 75, 155, 255};


    TileProperties beachProps;
    beachProps.moveSpeed = 0.9f;
    beachProps.canWalk  = true;
    beachProps.canSwim  = false;
    beachProps.canBuild = true;
    beachProps.isWater  = false;
    beachProps.tint     = {220, 205, 145, 255};
    beachProps.hasVegetation = false;
    beachProps.treeChance = 0.0f;


    TileProperties plainsProps;
    plainsProps.moveSpeed = 1.0f;
    plainsProps.canWalk  = true;
    plainsProps.canSwim  = false;
    plainsProps.canBuild = true;
    plainsProps.isWater  = false;
    plainsProps.tint     = {90, 170, 65, 255};
    plainsProps.hasVegetation = true;
    plainsProps.treeChance = 0.05f;


    TileProperties forestProps;
    forestProps.moveSpeed = 0.7f;
    forestProps.canWalk  = true;
    forestProps.canSwim  = false;
    forestProps.canBuild = true;
    forestProps.isWater  = false;
    forestProps.tint     = {35, 110, 30, 255};
    forestProps.hasVegetation = true;
    forestProps.treeChance = 0.8f;


    TileProperties hillsProps;
    hillsProps.moveSpeed = 0.6f;
    hillsProps.canWalk  = true;
    hillsProps.canSwim  = false;
    hillsProps.canBuild = true;
    hillsProps.isWater  = false;
    hillsProps.tint     = {135, 128, 88, 255};
    hillsProps.hasVegetation = true;
    hillsProps.treeChance = 0.3f;


    TileProperties mountainProps;
    mountainProps.moveSpeed = 0.3f;
    mountainProps.canWalk  = true;
    mountainProps.canSwim  = false;
    mountainProps.canBuild = false;
    mountainProps.isWater  = false;
    mountainProps.tint     = {100, 92, 82, 255};
    mountainProps.hasVegetation = true;
    mountainProps.treeChance = 0.1f;


    TileProperties snowProps;
    snowProps.moveSpeed = 0.2f;
    snowProps.canWalk  = true;
    snowProps.canSwim  = false;
    snowProps.canBuild = false;
    snowProps.isWater  = false;
    snowProps.tint     = {230, 236, 248, 255};
    snowProps.hasVegetation = false;
    snowProps.treeChance = 0.0f;

    biomes.clear();
    biomes.push_back({"Deep Water",    {12, 35, 100, 255},    0.0f,  0.22f, 0.0f, 1.0f, TileType::Water, deepWaterProps});
    biomes.push_back({"Shallow Water", {40, 95, 175, 255},    0.22f, 0.37f, 0.0f, 1.0f, TileType::Water, shallowWaterProps});
    biomes.push_back({"Beach",         {218, 204, 142, 255},  0.37f, 0.39f, 0.0f, 1.0f, TileType::Grass, beachProps});
    biomes.push_back({"Plains",        {95, 175, 68, 255},    0.39f, 0.45f, 0.0f, 1.0f, TileType::Grass, plainsProps});
    biomes.push_back({"Forest",        {38, 115, 32, 255},    0.45f, 0.67f, 0.0f, 1.0f, TileType::Grass, forestProps});
    biomes.push_back({"Hills",         {138, 132, 92, 255},   0.67f, 0.80f, 0.0f, 1.0f, TileType::Stone, hillsProps});
    biomes.push_back({"Mountain",      {108, 98, 88, 255},    0.80f, 0.92f, 0.0f, 1.0f, TileType::Stone, mountainProps});
    biomes.push_back({"Snow Peak",     {232, 238, 250, 255},  0.92f, 1.0f,  0.0f, 1.0f, TileType::Stone, snowProps});
}

// ─────────────────────── noise helpers ───────────────────────

float Terrain::fade(float t) const {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float Terrain::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

float Terrain::grad(int hash, float x, float y) const {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0.0f);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float Terrain::perlinNoise(float x, float y) const {
    static int permutation[512] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,

        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };

    int shift = seed % 256;
    int perm[512];
    for (int i = 0; i < 512; i++) {
        perm[i] = permutation[(i + shift) % 256];
    }

    int xi = (int)std::floor(x) & 255;
    int yi = (int)std::floor(y) & 255;

    float xf = x - std::floor(x);
    float yf = y - std::floor(y);

    float u = fade(xf);
    float v = fade(yf);

    int aa = perm[perm[xi]     + yi];
    int ab = perm[perm[xi]     + yi + 1];
    int ba = perm[perm[xi + 1] + yi];
    int bb = perm[perm[xi + 1] + yi + 1];

    float x1 = lerp(grad(aa, xf, yf),        grad(ba, xf - 1.0f, yf), u);
    float x2 = lerp(grad(ab, xf, yf - 1.0f), grad(bb, xf - 1.0f, yf - 1.0f), u);

    return lerp(x1, x2, v);
}

float Terrain::fbm(float x, float y, int octaves) const {
    float value     = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    float maxValue  = 0.0f;

    for (int i = 0; i < octaves; i++) {
        value    += amplitude * perlinNoise(x * frequency, y * frequency);
        maxValue += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return value / maxValue;
}

// ─────────────────────── generation ───────────────────────

void Terrain::generate() {
    float seedX = (float)((seed * 16807u) % 10000u);
    float seedY = (float)((seed * 48271u) % 10000u);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float nx = (float)x + seedX;
            float ny = (float)y + seedY;

            // --- Крупные формы (континенты) ---
            float continent_low  = fbm(nx * 0.0015f, ny * 0.0015f, 2);
            float continent_mid  = fbm(nx * 0.005f,  ny * 0.005f,  3);
            float continent_base = continent_low * 0.6f + continent_mid * 0.4f;
            continent_base = (continent_base + 1.0f) * 0.5f; // [0,1]
            continent_base = std::pow(continent_base, 1.2f); // контраст

            // --- Детали рельефа с warp ---
            float warpX = fbm(nx * 0.02f + 300.0f, ny * 0.02f + 300.0f, 2) * 25.0f;
            float warpY = fbm(nx * 0.02f + 700.0f, ny * 0.02f + 700.0f, 2) * 25.0f;
            float terrain_detail = fbm((nx + warpX) * 0.03f + 500.0f,
                                       (ny + warpY) * 0.03f + 500.0f, 5);

            // --- Горы (ridge) ---
            float ridge = fbm(nx * 0.012f + 1000.0f, ny * 0.012f + 1000.0f, 4);
            ridge = 1.0f - std::abs(ridge);
            ridge = std::pow(ridge, 2.5f);

            // --- Коэффициент суши (чтобы детали не создавали острова в воде) ---
            float land_factor = std::clamp((continent_base - 0.3f) / 0.7f, 0.0f, 1.0f);

            // --- Финальная высота ---
            float elevation = continent_base;
            elevation += (terrain_detail * 0.25f + ridge * 0.30f) * land_factor;
            elevation = std::clamp(elevation, 0.0f, 1.0f);
            elevation = std::pow(elevation, 1.3f); // дополнительный контраст

            Tile tile;
            tile.elevation  = elevation;
            tile.biomeIndex = getBiomeIndex(elevation, 0.0f, 0.5f);
            tile.type       = biomes[tile.biomeIndex].groundType;
            setTile(x, y, tile);
        }
    }
}

// ─────────────────────── biome / tile queries ───────────────────────

int Terrain::getBiomeIndex(float elevation, float moisture, float temperature) const {
    for (size_t i = 0; i < biomes.size(); ++i) {
        const Biome& b = biomes[i];
        if (elevation >= b.minElevation && elevation <= b.maxElevation) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

void Terrain::setTile(int x, int y, const Tile& tile) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        tiles[y * width + x] = tile;
    }
}

Tile& Terrain::getTile(int x, int y) {
    return tiles[y * width + x];
}

const Tile& Terrain::getTile(int x, int y) const {
    return tiles[y * width + x];
}

const Tile* Terrain::getTileAt(float worldX, float worldY) const {
    int tileSize = 8;
    int tx = (int)(worldX / tileSize);
    int ty = (int)(worldY / tileSize);
    if (tx < 0 || tx >= width || ty < 0 || ty >= height) return nullptr;
    return &getTile(tx, ty);
}

const TileProperties* Terrain::getTilePropertiesAt(float worldX, float worldY) const {
    const Tile* tile = getTileAt(worldX, worldY);
    if (!tile || tile->biomeIndex < 0) return nullptr;
    return &biomes[tile->biomeIndex].props;
}

const TileProperties& Terrain::getTileProperties(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        static TileProperties empty;
        return empty;
    }
    return biomes[getTile(x, y).biomeIndex].props;
}

const Biome* Terrain::getBiomeAt(float worldX, float worldY) const {
    const Tile* tile = getTileAt(worldX, worldY);
    if (!tile || tile->biomeIndex < 0) return nullptr;
    return &biomes[tile->biomeIndex];
}

bool Terrain::canWalk(float worldX, float worldY) const {
    const TileProperties* p = getTilePropertiesAt(worldX, worldY);
    return p ? p->canWalk : false;
}

bool Terrain::canBuild(float worldX, float worldY) const {
    const TileProperties* p = getTilePropertiesAt(worldX, worldY);
    return p ? p->canBuild : false;
}

float Terrain::getMoveSpeedAt(float worldX, float worldY) const {
    const TileProperties* p = getTilePropertiesAt(worldX, worldY);
    return p ? p->moveSpeed : 1.0f;
}

bool Terrain::isPassable(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return getTileProperties(x, y).canWalk;
}

// UNUSED: implemented but never called
// bool Terrain::findNearestPassable(int targetX, int targetY,
//                                    int& outX, int& outY, int maxRadius) const {
//     for (int r = 1; r <= maxRadius; ++r) {
//         for (int dy = -r; dy <= r; ++dy) {
//             for (int dx = -r; dx <= r; ++dx) {
//                 if (abs(dx) != r && abs(dy) != r) continue;
//                 int nx = targetX + dx;
//                 int ny = targetY + dy;
//                 if (isPassable(nx, ny)) {
//                     outX = nx;
//                     outY = ny;
//                     return true;
//                 }
//             }
//         }
//     }
//     return false;
// }

// ─────────────────────── rendering ───────────────────────

void Terrain::draw() const {
    const int tileSize = 8;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Tile& tile = getTile(x, y);
            if (tile.biomeIndex < 0) {
                DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize, BLACK);
                continue;
            }

            const Biome& biome = biomes[tile.biomeIndex];
            float r = (float)biome.color.r;
            float g = (float)biome.color.g;
            float b = (float)biome.color.b;

            // Where this tile sits inside its own biome (0 = low edge, 1 = high)
            float biomeSpan = biome.maxElevation - biome.minElevation;
            float biomeT = (biomeSpan > 0.001f)
                ? (tile.elevation - biome.minElevation) / biomeSpan
                : 0.5f;

            // ── Two cheap per-tile hashes for colour jitter ──
            float h1 = std::sin((float)x * 12.9898f + (float)y * 78.233f)  * 43758.5453f;
            h1 = h1 - std::floor(h1);                       // [0,1]
            float h2 = std::sin((float)x * 63.726f  + (float)y * 10.873f)  * 28462.234f;
            h2 = h2 - std::floor(h2);

            float jitter = (h1 - 0.5f) * 2.0f;             // [-1,1]

            if (biome.props.isWater) {
                // ── Water: depth shading ──
                // Deep = darker & more saturated; shallow = lighter & greener
                float depth = 1.0f - biomeT;                // 1 = deepest
                r = r * (0.55f + biomeT * 0.45f);
                g = g * (0.60f + biomeT * 0.40f);
                b = b * (0.70f + biomeT * 0.30f);

                // Specular-ish sparkle on shallow water
                if (biomeT > 0.6f) {
                    float sparkle = h2 * (biomeT - 0.6f) * 40.0f;
                    r += sparkle;
                    g += sparkle;
                    b += sparkle * 1.3f;
                }

                // Very subtle per-tile ripple
                r += jitter * 6.0f;
                g += jitter * 8.0f;
                b += jitter * 10.0f;

            } else {
                // ── Land biomes ──

                // Gentle brightness ramp across biome elevation band
                float shade = 0.88f + biomeT * 0.12f;
                r *= shade;
                g *= shade;
                b *= shade;

                // Beach: warmer / cooler sand patches
                if (tile.biomeIndex == 2) {                  // Beach
                    float warmth = jitter * 14.0f;
                    r += warmth;
                    g += warmth * 0.7f;
                    b -= std::abs(warmth) * 0.5f;
                    // Wet sand near water edge
                    if (biomeT < 0.3f) {
                        float wet = (0.3f - biomeT) / 0.3f;
                        r -= wet * 25.0f;
                        g -= wet * 15.0f;
                        b += wet * 10.0f;
                    }
                }
                // Plains: yellow-green variation
                else if (tile.biomeIndex == 3) {             // Plains
                    r += jitter * 18.0f + h2 * 10.0f;
                    g += jitter * 12.0f;
                    b += jitter * 6.0f;
                }
                // Forest: dark / light canopy patches
                else if (tile.biomeIndex == 4) {             // Forest
                    float canopy = jitter * 16.0f;
                    r += canopy * 0.4f;
                    g += canopy;
                    b += canopy * 0.3f;
                }
                // Hills+: general rocky jitter
                else {
                    r += jitter * 12.0f;
                    g += jitter * 10.0f;
                    b += jitter * 8.0f;
                }

                // ── Height-based whitening (snow/frost bleed) ──
                if (tile.elevation > 0.68f) {
                    float t = (tile.elevation - 0.68f) / 0.32f;  // 0→1
                    float white = std::pow(t, 1.6f) * 0.55f;
                    r = r + (255.0f - r) * white;
                    g = g + (255.0f - g) * white;
                    b = b + (255.0f - b) * white;
                }
            }

            // Clamp color components to valid range
            r = std::clamp(r, 0.0f, 255.0f);
            g = std::clamp(g, 0.0f, 255.0f);
            b = std::clamp(b, 0.0f, 255.0f);

            Color finalColor = { (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
            DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize, finalColor);
        }
    }
}
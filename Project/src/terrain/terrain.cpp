#include "terrain/terrain.h"

#include <algorithm>
#include <cstdlib>
#include <cmath>

namespace {
    float smoothstep(float edge0, float edge1, float x) {
        float t = (x - edge0) / (edge1 - edge0);
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        return t * t * (3.0f - 2.0f * t);
    }

    float clamp(float x, float minVal, float maxVal) {
        if (x < minVal) return minVal;
        if (x > maxVal) return maxVal;
        return x;
    }

    float distToLineSegment(float px, float py, float x1, float y1, float x2, float y2) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float len2 = dx * dx + dy * dy;
        if (len2 < 0.0001f) return sqrtf((px - x1) * (px - x1) + (py - y1) * (py - y1));
        float t = ((px - x1) * dx + (py - y1) * dy) / len2;
        t = clamp(t, 0.0f, 1.0f);
        float nearX = x1 + t * dx;
        float nearY = y1 + t * dy;
        return sqrtf((px - nearX) * (px - nearX) + (py - nearY) * (py - nearY));
    }
}

Terrain::Terrain(int width, int height, unsigned int seed)
    : width(width), height(height), seed(seed), tiles(width * height) {

    TileProperties deepOceanProps{0.0f, false, true, false, true, false, {0, 40, 100, 255}, 0.0f, 0.0f, VegetationType::None, OreType::None, TerrainFeature::None, {0,0,0,0}, 100.0f};
    TileProperties oceanProps{0.0f, false, true, false, true, false, {0, 80, 180, 255}, 0.0f, 0.0f, VegetationType::None, OreType::None, TerrainFeature::None, {0,0,0,0}, 100.0f};
    TileProperties beachProps{0.8f, true, false, true, false, false, {238, 200, 160, 255}, 0.0f, 0.0f, VegetationType::None, OreType::None, TerrainFeature::Beach, {200, 180, 140, 255}, 0.0f};
    TileProperties desertProps{0.9f, true, false, true, false, false, {237, 201, 175, 255}, 0.02f, 0.0f, VegetationType::Cactus, OreType::None, TerrainFeature::None, {0,0,0,0}, 0.0f};
    TileProperties savannaProps{0.85f, true, false, true, false, true, {160, 140, 70, 255}, 0.05f, 0.0f, VegetationType::PalmTree, OreType::None, TerrainFeature::None, {0,0,0,0}, 0.0f};
    TileProperties grasslandProps{1.0f, true, false, true, false, true, {70, 140, 70, 255}, 0.08f, 0.0f, VegetationType::OakTree, OreType::None, TerrainFeature::None, {0,0,0,0}, 0.0f};
    TileProperties forestProps{0.7f, true, false, true, false, true, {30, 100, 30, 255}, 0.5f, 0.0f, VegetationType::OakTree, OreType::None, TerrainFeature::None, {0,0,0,0}, 0.0f};
    TileProperties taigaProps{0.6f, true, false, true, false, true, {35, 70, 50, 255}, 0.4f, 0.0f, VegetationType::PineTree, OreType::None, TerrainFeature::None, {0,0,0,0}, 0.0f};
    TileProperties jungleProps{0.5f, true, false, true, false, true, {20, 80, 20, 255}, 0.6f, 0.0f, VegetationType::Bush, OreType::None, TerrainFeature::Swamp, {60, 100, 60, 255}, 0.0f};
    TileProperties swampProps{0.4f, true, false, true, false, false, {60, 80, 50, 255}, 0.15f, 0.0f, VegetationType::Bush, OreType::None, TerrainFeature::Swamp, {50, 70, 40, 255}, 0.0f};
    TileProperties tundraProps{0.5f, true, false, true, false, true, {180, 170, 160, 255}, 0.05f, 0.0f, VegetationType::Bush, OreType::None, TerrainFeature::None, {0,0,0,0}, 0.0f};
    TileProperties mountainProps{0.3f, true, false, true, false, false, {100, 100, 110, 255}, 0.0f, 0.12f, VegetationType::None, OreType::Coal, TerrainFeature::Cliff, {80, 80, 90, 255}, 0.0f};
    TileProperties highMountainProps{0.5f, true, false, true, false, false, {130, 130, 140, 255}, 0.0f, 0.08f, VegetationType::None, OreType::Iron, TerrainFeature::Cliff, {110, 110, 120, 255}, 0.0f};
    TileProperties snowProps{0.5f, true, false, true, false, false, {220, 230, 240, 255}, 0.0f, 0.02f, VegetationType::None, OreType::None, TerrainFeature::None, {0,0,0,0}, 0.0f};
    TileProperties volcanicProps{0.5f, true, false, true, false, false, {80, 30, 30, 255}, 0.0f, 0.05f, VegetationType::None, OreType::Gold, TerrainFeature::Volcano, {200, 50, 20, 255}, 50.0f};
    TileProperties canyonProps{0.4f, true, false, true, false, false, {160, 100, 60, 255}, 0.0f, 0.0f, VegetationType::None, OreType::Copper, TerrainFeature::Canyon, {140, 80, 50, 255}, 0.0f};
    TileProperties oasisProps{1.0f, true, false, true, false, false, {100, 180, 150, 255}, 0.3f, 0.0f, VegetationType::PalmTree, OreType::None, TerrainFeature::Oasis, {80, 160, 120, 255}, 0.0f};

    biomes = {
        {"DeepOcean", {0, 80, 160, 255},   0.0f, 0.25f, 0.0f, 1.0f, 0.0f, 1.0f, TileType::Water, deepOceanProps},
        {"Ocean",     {0, 110, 190, 255},  0.25f, 0.36f, 0.0f, 1.0f, 0.0f, 1.0f, TileType::Water, oceanProps},
        {"Beach",     {238, 232, 170, 255},0.36f, 0.42f, 0.0f, 1.0f, 0.0f, 1.0f, TileType::Sand,  beachProps},

        {"Desert",    {212, 160, 23, 255}, 0.42f, 0.58f, 0.0f, 0.25f, 0.5f, 1.0f, TileType::Sand,  desertProps},
        {"Savanna",   {180, 140, 40, 255}, 0.42f, 0.58f, 0.25f, 0.45f, 0.5f, 1.0f, TileType::Grass, savannaProps},

        {"Grassland", {64, 128, 64, 255},  0.42f, 0.58f, 0.4f, 0.65f, 0.3f, 0.8f, TileType::Grass, grasslandProps},
        {"Forest",    {32, 96, 32, 255},   0.42f, 0.58f, 0.55f, 0.85f, 0.3f, 0.7f, TileType::Grass, forestProps},
        {"Jungle",    {20, 80, 20, 255},   0.42f, 0.58f, 0.75f, 1.0f, 0.6f, 1.0f, TileType::Grass, jungleProps},

        {"Swamp",     {60, 80, 50, 255},   0.35f, 0.50f, 0.85f, 1.0f, 0.4f, 1.0f, TileType::Dirt, swampProps},

        {"Taiga",     {40, 80, 60, 255},   0.50f, 0.62f, 0.5f, 1.0f, 0.0f, 0.4f, TileType::Dirt,  taigaProps},
        {"Tundra",    {180, 170, 160, 255},0.50f, 0.62f, 0.4f, 0.8f, 0.0f, 0.35f, TileType::Dirt, tundraProps},

        {"Mountain",  {48, 48, 48, 255},   0.62f, 0.80f, 0.0f, 1.0f, 0.0f, 0.9f, TileType::Stone, mountainProps},
        {"HighMountain",{40, 40, 45, 255},0.80f, 0.92f, 0.0f, 1.0f, 0.0f, 0.6f, TileType::Stone, highMountainProps},

        {"Volcanic",  {80, 30, 30, 255},   0.65f, 0.80f, 0.0f, 0.4f, 0.7f, 1.0f, TileType::Lava, volcanicProps},

        {"Snow",      {224, 224, 224, 255},0.85f, 1.0f,  0.0f, 1.0f, 0.0f, 0.35f, TileType::Snow,  snowProps},

        {"Canyon",    {160, 100, 60, 255},  0.50f, 0.70f, 0.0f, 0.3f, 0.6f, 1.0f, TileType::Stone, canyonProps},
        {"Oasis",     {100, 180, 150, 255}, 0.42f, 0.50f, 0.0f, 0.2f, 0.7f, 1.0f, TileType::Sand, oasisProps},
    };
}

int Terrain::getBiomeIndex(float elevation, float moisture, float temp) const {
    for (size_t i = 0; i < biomes.size(); ++i) {
        const Biome& b = biomes[i];
        if (elevation >= b.minElevation && elevation <= b.maxElevation &&
            moisture >= b.minMoisture && moisture <= b.maxMoisture &&
            temp >= b.minTemp && temp <= b.maxTemp) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

void Terrain::generate() {
    const int regionsX = 6;
    const int regionsY = 5;
    int regionWidth = width / regionsX;
    int regionHeight = height / regionsY;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Tile& tile = getTile(x, y);

            int regionX = x / regionWidth;
            int regionY = y / regionHeight;
            if (regionX >= regionsX) regionX = regionsX - 1;
            if (regionY >= regionsY) regionY = regionsY - 1;

            tile.elevation = 0.5f;
            tile.moisture = 0.5f;
            tile.temperature = 0.5f;
            tile.feature = TerrainFeature::None;
            tile.vegetation = {};
            tile.ore = {};

            if (regionY == 0) {
                if (regionX == 0) tile.biomeIndex = 0;
                else if (regionX == 1) tile.biomeIndex = 1;
                else tile.biomeIndex = 2;
            } else if (regionY == 1) {
                if (regionX == 0) tile.biomeIndex = 3;
                else if (regionX == 1) tile.biomeIndex = 4;
                else if (regionX == 2) tile.biomeIndex = 16;
                else tile.biomeIndex = 17;
            } else if (regionY == 2) {
                if (regionX == 0) tile.biomeIndex = 5;
                else if (regionX == 1) tile.biomeIndex = 6;
                else if (regionX == 2) tile.biomeIndex = 7;
                else if (regionX == 3) tile.biomeIndex = 8;
                else tile.biomeIndex = 16;
            } else if (regionY == 3) {
                if (regionX == 0) tile.biomeIndex = 9;
                else if (regionX == 1) tile.biomeIndex = 10;
                else if (regionX == 2) tile.biomeIndex = 11;
                else if (regionX == 3) tile.biomeIndex = 12;
                else tile.biomeIndex = 13;
            } else {
                if (regionX == 0) tile.biomeIndex = 14;
                else if (regionX == 1) tile.biomeIndex = 15;
                else tile.biomeIndex = 14;
            }

            const Biome& biome = biomes[tile.biomeIndex];
            tile.type = biome.groundType;

            if (biome.props.preferredTree != VegetationType::None) {
                if ((x + y) % 7 == 0) {
                    tile.vegetation.type = biome.props.preferredTree;
                    tile.vegetation.yieldAmount = 3;
                    tile.vegetation.yieldName = "wood";
                    tile.vegetation.isHarvestable = true;
                }
            }

            if (biome.props.oreType != OreType::None) {
                if ((x * 3 + y * 7) % 11 == 0) {
                    tile.ore.type = biome.props.oreType;
                    tile.ore.amount = 3;
                    tile.ore.yieldName = "ore";
                    tile.ore.isHarvestable = true;
                }
            }

            if (biome.props.feature != TerrainFeature::None) {
                if ((x + y * 2) % 13 == 0) {
                    tile.feature = biome.props.feature;
                }
            }

            if (biome.props.feature == TerrainFeature::None && biome.groundType == TileType::Stone) {
                if ((x * 5 + y) % 17 == 0) {
                    tile.feature = TerrainFeature::Rock;
                }
            }
        }
    }
}

void Terrain::generateActualPlayMap() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Tile& tile = getTile(x, y);
            tile.biomeIndex = 0;
            tile.elevation = 0.0f;
            tile.moisture = 0.0f;
            tile.temperature = 0.0f;
            tile.feature = TerrainFeature::None;
            tile.vegetation = {};
            tile.ore = {};
            tile.type = TileType::Water;
        }
    }
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
    int tileSize = 20;
    int tx = (int)(worldX / tileSize);
    int ty = (int)(worldY / tileSize);
    if (tx < 0 || tx >= width || ty < 0 || ty >= height) {
        return nullptr;
    }
    return &getTile(tx, ty);
}

const TileProperties* Terrain::getTilePropertiesAt(float worldX, float worldY) const {
    const Biome* biome = getBiomeAt(worldX, worldY);
    if (!biome) return nullptr;
    return &biome->props;
}

const TileProperties& Terrain::getTileProperties(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        static TileProperties empty;
        return empty;
    }
    const Biome& biome = biomes[getTile(x, y).biomeIndex];
    return biome.props;
}

const VegetationData* Terrain::getVegetationAt(float worldX, float worldY) const {
    const Tile* tile = getTileAt(worldX, worldY);
    if (!tile || tile->vegetation.type == VegetationType::None) {
        return nullptr;
    }
    return &tile->vegetation;
}

const OreData* Terrain::getOreAt(float worldX, float worldY) const {
    const Tile* tile = getTileAt(worldX, worldY);
    if (!tile || tile->ore.type == OreType::None) {
        return nullptr;
    }
    return &tile->ore;
}

const Biome* Terrain::getBiomeAt(float worldX, float worldY) const {
    const Tile* tile = getTileAt(worldX, worldY);
    if (!tile || tile->biomeIndex < 0) {
        return nullptr;
    }
    return &biomes[tile->biomeIndex];
}

bool Terrain::canWalk(float worldX, float worldY) const {
    const TileProperties* props = getTilePropertiesAt(worldX, worldY);
    if (!props) return false;
    return props->canWalk;
}

bool Terrain::canBuild(float worldX, float worldY) const {
    const TileProperties* props = getTilePropertiesAt(worldX, worldY);
    if (!props) return false;
    return props->canBuild;
}

float Terrain::getDamageAt(float worldX, float worldY) const {
    const TileProperties* props = getTilePropertiesAt(worldX, worldY);
    if (!props) return 0.0f;
    return props->damage;
}

bool Terrain::isPassable(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    const Tile& tile = getTile(x, y);
    const TileProperties& props = getTileProperties(x, y);
    return props.canWalk;
}

bool Terrain::findNearestPassable(int targetX, int targetY, int& outX, int& outY, int maxRadius) const {
    for (int r = 1; r <= maxRadius; ++r) {
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (abs(dx) != r && abs(dy) != r) continue;
                int nx = targetX + dx;
                int ny = targetY + dy;
                if (isPassable(nx, ny)) {
                    outX = nx;
                    outY = ny;
                    return true;
                }
            }
        }
    }
    return false;
}

float Terrain::distToRegion(int px, int py, int rx1, int ry1, int rx2, int ry2) const {
    return distToLineSegment(px, py, rx1, ry1, rx2, ry2);
}

void Terrain::carveRegion(int x1, int y1, int x2, int y2, float edgeFade, int biomeIdx) {
    for (int y = y1; y < y2 && y < height; ++y) {
        for (int x = x1; x < x2 && x < width; ++x) {
            float edgeDist = distToRegion(x, y, x1, y1, x2, y2);
            if (edgeDist < edgeFade * (x2 - x1)) {
                getTile(x, y).biomeIndex = biomeIdx;
            }
        }
    }
}

void Terrain::draw() const {
    const int tileSize = 20;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Tile& tile = getTile(x, y);
            Color color = (tile.biomeIndex >= 0) ? biomes[tile.biomeIndex].color : MAGENTA;

            DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize, color);

            if (tile.feature == TerrainFeature::Cliff) {
                DrawRectangle(x * tileSize + tileSize/2 - 1, y * tileSize, 2, tileSize, {30, 30, 35, 255});
            } else if (tile.feature == TerrainFeature::Rock) {
                DrawCircle(x * tileSize + tileSize/2, y * tileSize + tileSize/2, tileSize/3, {50, 50, 55, 255});
            } else if (tile.feature == TerrainFeature::Beach) {
                DrawRectangle(x * tileSize + tileSize/2 - 1, y * tileSize + tileSize/2 - 1, 2, 2, {200, 190, 150, 255});
            } else if (tile.feature == TerrainFeature::Swamp) {
                DrawCircle(x * tileSize + tileSize/2, y * tileSize + tileSize/2, tileSize/2, {45, 60, 40, 180});
            } else if (tile.feature == TerrainFeature::Volcano) {
                DrawCircle(x * tileSize + tileSize/2, y * tileSize + tileSize/2, tileSize/2, {255, 60, 10, 255});
            } else if (tile.feature == TerrainFeature::Oasis) {
                DrawCircle(x * tileSize + tileSize/2, y * tileSize + tileSize/2, tileSize/2, {50, 130, 100, 200});
            }

            if (tile.vegetation.type != VegetationType::None) {
                Color vegColor = {20, 80, 20, 255};
                if (tile.vegetation.type == VegetationType::PineTree) vegColor = {15, 50, 25, 255};
                else if (tile.vegetation.type == VegetationType::PalmTree) vegColor = {80, 120, 40, 255};
                else if (tile.vegetation.type == VegetationType::Cactus) vegColor = {50, 100, 50, 255};
                else if (tile.vegetation.type == VegetationType::Bush) vegColor = {40, 80, 30, 255};
                DrawCircle(x * tileSize + tileSize / 2, y * tileSize + tileSize / 2, tileSize / 2, vegColor);
            }

            if (tile.ore.type != OreType::None) {
                Color oreColor = GRAY;
                if (tile.ore.type == OreType::Coal) oreColor = {20, 20, 20, 255};
                else if (tile.ore.type == OreType::Iron) oreColor = {120, 80, 60, 255};
                else if (tile.ore.type == OreType::Gold) oreColor = {220, 180, 40, 255};
                else if (tile.ore.type == OreType::Copper) oreColor = {150, 100, 60, 255};
                DrawRectangle(x * tileSize + tileSize/4, y * tileSize + tileSize/4, tileSize/2, tileSize/2, oreColor);
            }
        }
    }
}
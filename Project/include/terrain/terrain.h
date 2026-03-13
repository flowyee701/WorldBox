#pragma once

#include <string>
#include <vector>

#include <raylib.h>

enum class TileType {
    None,
    Grass,
    Dirt,
    Sand,
    Stone,
    Water,
    Snow,
    Lava,
};

enum class VegetationType {
    None,
    OakTree,
    PineTree,
    PalmTree,
    Bush,
    Cactus,
    Bamboo,
};

enum class OreType {
    None,
    Coal,
    Iron,
    Gold,
    Diamond,
    Copper,
};

enum class TerrainFeature {
    None,
    Cliff,
    Rock,
    Beach,
    Swamp,
    Volcano,
    Canyon,
    Oasis,
};

struct VegetationData {
    VegetationType type = VegetationType::None;
    bool isHarvestable = true;
    int yieldAmount = 1;
    std::string yieldName = "wood";
};

struct OreData {
    OreType type = OreType::None;
    int amount = 0;
    bool isHarvestable = true;
    std::string yieldName = "ore";
};

struct TileProperties {
    float moveSpeed = 1.0f;
    bool canWalk = true;
    bool canSwim = false;
    bool canBuild = true;
    bool isWater = false;
    bool hasVegetation = false;
    Color tint = WHITE;
    
    float treeChance = 0.0f;
    float oreChance = 0.0f;
    VegetationType preferredTree = VegetationType::None;
    OreType oreType = OreType::None;
    
    TerrainFeature feature = TerrainFeature::None;
    Color featureColor = {0, 0, 0, 0};
    
    float damage = 0.0f;
};

struct Biome {
    Biome() = default;
    Biome(std::string name_, Color color_, float minE, float maxE, float minT, float maxT, TileType ground, TileProperties props_)
        : name(std::move(name_)), color(color_), minElevation(minE), maxElevation(maxE),
          minTemp(minT), maxTemp(maxT), groundType(ground), props(std::move(props_)) {}

    std::string name;
    Color color;
    float minElevation;
    float maxElevation;
    float minMoisture;
    float maxMoisture;
    float minTemp;
    float maxTemp;
    TileType groundType;
    
    TileProperties props;
};

struct Tile {
    TileType type = TileType::None;
    int biomeIndex = -1;
    float elevation = 0.0f;
    float moisture = 0.0f;
    float temperature = 0.0f;
    
    VegetationData vegetation;
    OreData ore;
    TerrainFeature feature = TerrainFeature::None;
};

class Terrain {
public:
    Terrain() = default;
    Terrain(int width, int height, unsigned int seed);
    ~Terrain() = default;

    void setTile(int x, int y, const Tile& tile);

    // void Terrain::setTile(int x, int y, const Tile& tile) {
    //     if (x >= 0 && x < width && y >= 0 && y < height) {
    //         tiles[y * width + x] = tile;
    //     }
    // }

    void generate();

    // oid Terrain::generate() {
    //     float seedX = (float)((seed * 16807u) % 10000u);
    //     float seedY = (float)((seed * 48271u) % 10000u);
    //
    //     for (int y = 0; y < height; y++) {
    //         for (int x = 0; x < width; x++) {
    //             float nx = (float)x + seedX;
    //             float ny = (float)y + seedY;
    //
    //             // --- Крупные формы (континенты) ---
    //             float continent_low  = fbm(nx * 0.0015f, ny * 0.0015f, 2);
    //             float continent_mid  = fbm(nx * 0.005f,  ny * 0.005f,  3);
    //             float continent_base = continent_low * 0.6f + continent_mid * 0.4f;
    //             continent_base = (continent_base + 1.0f) * 0.5f; // [0,1]
    //             continent_base = std::pow(continent_base, 1.2f); // контраст
    //
    //             // --- Детали рельефа с warp ---
    //             float warpX = fbm(nx * 0.02f + 300.0f, ny * 0.02f + 300.0f, 2) * 25.0f;
    //             float warpY = fbm(nx * 0.02f + 700.0f, ny * 0.02f + 700.0f, 2) * 25.0f;
    //             float terrain_detail = fbm((nx + warpX) * 0.03f + 500.0f,
    //                                        (ny + warpY) * 0.03f + 500.0f, 5);
    //
    //             // --- Горы (ridge) ---
    //             float ridge = fbm(nx * 0.012f + 1000.0f, ny * 0.012f + 1000.0f, 4);
    //             ridge = 1.0f - std::abs(ridge);
    //             ridge = std::pow(ridge, 2.5f);
    //
    //             // --- Коэффициент суши (чтобы детали не создавали острова в воде) ---
    //             float land_factor = std::clamp((continent_base - 0.3f) / 0.7f, 0.0f, 1.0f);
    //
    //             // --- Финальная высота ---
    //             float elevation = continent_base;
    //             elevation += (terrain_detail * 0.25f + ridge * 0.30f) * land_factor;
    //             elevation = std::clamp(elevation, 0.0f, 1.0f);
    //             elevation = std::pow(elevation, 1.3f); // дополнительный контраст
    //
    //             Tile tile;
    //             tile.elevation  = elevation;
    //             tile.biomeIndex = getBiomeIndex(elevation, 0.0f, 0.5f);
    //             tile.type       = biomes[tile.biomeIndex].groundType;
    //             setTile(x, y, tile);
    //         }
    //     }
    // }

  // void addArchipelago(float cx, float cy, float sizeX, float sizeY, int islandCount, int seed);
    Tile& getTile(int x, int y);
    const Tile& getTile(int x, int y) const;

    // Tile& Terrain::getTile(int x, int y) {
    //     return tiles[y * width + x];
    // }


    
    const Tile* getTileAt(float worldX, float worldY) const;

    // const Tile* Terrain::getTileAt(float worldX, float worldY) const {
    //     int tileSize = 8;
    //     int tx = (int)(worldX / tileSize);
    //     int ty = (int)(worldY / tileSize);
    //     if (tx < 0 || tx >= width || ty < 0 || ty >= height) return nullptr;
    //     return &getTile(tx, ty);
    // }

    const TileProperties* getTilePropertiesAt(float worldX, float worldY) const;

    // const TileProperties* Terrain::getTilePropertiesAt(float worldX, float worldY) const {
    //     const Tile* tile = getTileAt(worldX, worldY);
    //     if (!tile || tile->biomeIndex < 0) return nullptr;
    //     return &biomes[tile->biomeIndex].props;
    // }

    const Biome* getBiomeAt(float worldX, float worldY) const;

    // const Biome* Terrain::getBiomeAt(float worldX, float worldY) const {
    //     const Tile* tile = getTileAt(worldX, worldY);
    //     if (!tile || tile->biomeIndex < 0) return nullptr;
    //     return &biomes[tile->biomeIndex];
    // }

    bool canWalk(float worldX, float worldY) const;

    // bool Terrain::canWalk(float worldX, float worldY) const {
    //     const TileProperties* p = getTilePropertiesAt(worldX, worldY);
    //     return p ? p->canWalk : false;
    // }

    bool canBuild(float worldX, float worldY) const;

    // bool Terrain::canBuild(float worldX, float worldY) const {
    //     const TileProperties* p = getTilePropertiesAt(worldX, worldY);
    //     return p ? p->canBuild : false;
    // }


// UNUSED: declared but never implemented
//     float getDamageAt(float worldX, float worldY) const;



    float getMoveSpeedAt(float worldX, float worldY) const;

    // float Terrain::getMoveSpeedAt(float worldX, float worldY) const {
    //     const TileProperties* p = getTilePropertiesAt(worldX, worldY);
    //     return p ? p->moveSpeed : 1.0f;
    // }
    
    const TileProperties& getTileProperties(int x, int y) const;

    // const TileProperties& Terrain::getTileProperties(int x, int y) const {
    //     if (x < 0 || x >= width || y < 0 || y >= height) {
    //         static TileProperties empty;
    //         return empty;
    //     }
    //     return biomes[getTile(x, y).biomeIndex].props;
    // }

// UNUSED: declared but never implemented
//     const VegetationData* getVegetationAt(float worldX, float worldY) const;



// UNUSED: declared but never implemented
//     const OreData* getOreAt(float worldX, float worldY) const;
    
    void draw() const;

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    bool isPassable(int x, int y) const;
// UNUSED: implemented but never called
//     bool findNearestPassable(int targetX, int targetY, int& outX, int& outY, int maxRadius = 10) const;

    const std::vector<Biome>& getBiomes() const { return biomes; }
    int getBiomeCount() const { return (int)biomes.size(); }

private:
    int width;
    int height;
    unsigned int seed;
    std::vector<Tile> tiles;
    std::vector<Biome> biomes;

    int getBiomeIndex(float elevation, float moisture, float temperature) const;

    float fade(float t) const;
    float lerp(float a, float b, float t) const;
    float grad(int hash, float x, float y) const;
    float perlinNoise(float x, float y) const;
    float fbm(float x, float y, int octaves) const;

    float distToRegion(int px, int py, int rx1, int ry1, int rx2, int ry2) const;
    void carveRegion(int x1, int y1, int x2, int y2, float edgeFade, int biomeIdx);
};



//

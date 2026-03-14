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
    void generate();
    void generateActualPlayMap();
    void addArchipelago(float cx, float cy, float sizeX, float sizeY, int islandCount, int seed);
    Tile& getTile(int x, int y);
    const Tile& getTile(int x, int y) const;
    
    const Tile* getTileAt(float worldX, float worldY) const;
    const TileProperties* getTilePropertiesAt(float worldX, float worldY) const;
    const Biome* getBiomeAt(float worldX, float worldY) const;
    bool canWalk(float worldX, float worldY) const;
    bool canBuild(float worldX, float worldY) const;
    float getDamageAt(float worldX, float worldY) const;
    float getMoveSpeedAt(float worldX, float worldY) const;
    
    const TileProperties& getTileProperties(int x, int y) const;
    
    const VegetationData* getVegetationAt(float worldX, float worldY) const;
    const OreData* getOreAt(float worldX, float worldY) const;
    
    void draw() const;

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    bool isPassable(int x, int y) const;
    bool findNearestPassable(int targetX, int targetY, int& outX, int& outY, int maxRadius = 10) const;

    const std::vector<Biome>& getBiomes() const { return biomes; }
    int getBiomeCount() const { return (int)biomes.size(); }
    int getBiomeIndex(float elevation, float moisture, float temperature) const;

private:
    int width;
    int height;
    unsigned int seed;
    std::vector<Tile> tiles;
    std::vector<Biome> biomes;

    float fade(float t) const;
    float lerp(float a, float b, float t) const;
    float grad(int hash, float x, float y) const;
    float perlinNoise(float x, float y) const;
    float fbm(float x, float y, int octaves) const;

    float fade(float t) const;
    float lerp(float a, float b, float t) const;
    float grad(int hash, float x, float y) const;
    float perlinNoise(float x, float y) const;
    float fbm(float x, float y, int octaves) const;

    float distToRegion(int px, int py, int rx1, int ry1, int rx2, int ry2) const;
    void carveRegion(int x1, int y1, int x2, int y2, float edgeFade, int biomeIdx);
};

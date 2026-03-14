#include <gtest/gtest.h>
#include "terrain/terrain.h"
#include "environment/world.h"
#include "environment/Plant.h"

TEST(TerrainTest, InitDefaultDoesNotCrash) {
    Terrain terrain;
}

TEST(TerrainTest, InitWithParamsDoesNotCrash) {
    Terrain terrain(100, 100, 42);
    terrain.generate();
}

TEST(TerrainTest, GetTileDoesNotCrash) {
    Terrain terrain(100, 100, 42);
    terrain.generate();
    Tile& tile = terrain.getTile(50, 50);
    (void)tile;
}

TEST(TerrainTest, GetTileOutOfBoundsDoesNotCrash) {
    Terrain terrain(100, 100, 42);
    terrain.generate();
    Tile& tile = terrain.getTile(200, 200);
    (void)tile;
}

TEST(TerrainTest, GetTilePropertiesDoesNotCrash) {
    Terrain terrain(100, 100, 42);
    terrain.generate();
    const TileProperties& props = terrain.getTileProperties(50, 50);
    (void)props;
}

TEST(TerrainTest, CanWalkDoesNotCrash) {
    Terrain terrain(100, 100, 42);
    terrain.generate();
    bool canWalk = terrain.canWalk(50.0f, 50.0f);
    (void)canWalk;
}

TEST(TerrainTest, CanBuildDoesNotCrash) {
    Terrain terrain(100, 100, 42);
    terrain.generate();
    bool canBuild = terrain.canBuild(50.0f, 50.0f);
    (void)canBuild;
}

TEST(TerrainTest, IsPassableDoesNotCrash) {
    Terrain terrain(100, 100, 42);
    terrain.generate();
    bool passable = terrain.isPassable(50, 50);
    (void)passable;
}

TEST(PlantTest, InitDoesNotCrash) {
    Plant plant({100.0f, 100.0f});
}

TEST(TileTest, DefaultConstructionDoesNotCrash) {
    Tile tile;
    (void)tile;
}

TEST(TilePropertiesTest, DefaultConstructionDoesNotCrash) {
    TileProperties props;
    (void)props;
}

TEST(BiomeTest, DefaultConstructionDoesNotCrash) {
    Biome biome;
    (void)biome;
}

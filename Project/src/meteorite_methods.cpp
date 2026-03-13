// Meteorite methods implementation
#include "environment/world.h"
#include "environment/Meteorite.h"
#include <cmath>

void World::LoadMeteoriteSprites() {
    for (int i = 0; i < METEORITE_FRAMES; i++) {
        meteoriteTexLoaded[i] = false;
        meteoriteTex[i] = {0};
    }
    
    for (int frame = 0; frame < METEORITE_FRAMES; frame++) {
        Image img = GenImageColor(64, 64, BLANK);
        
        float pulse = 0.5f + 0.5f * sinf((float)frame * PI * 2.0f / METEORITE_FRAMES);
        
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                float dx = x - 32.0f;
                float dy = y - 32.0f;
                float dist = sqrtf(dx*dx + dy*dy);
                
                if (dist < 8.0f) {
                    Color core = {
                        (unsigned char)(255),
                        (unsigned char)(200 + 55 * pulse),
                        (unsigned char)(100 + 50 * pulse),
                        255
                    };
                    ImageDrawPixel(&img, x, y, core);
                }
                else if (dist < 14.0f) {
                    Color inner = {
                        (unsigned char)(200 + 55 * pulse),
                        (unsigned char)(100 + 50 * pulse),
                        (unsigned char)(30),
                        255
                    };
                    ImageDrawPixel(&img, x, y, inner);
                }
                else if (dist < 20.0f) {
                    Color mid = {
                        (unsigned char)(160),
                        (unsigned char)(60 + frame * 10),
                        (unsigned char)(20),
                        255
                    };
                    ImageDrawPixel(&img, x, y, mid);
                }
                else if (dist < 26.0f) {
                    unsigned char alpha = (unsigned char)(255 * (1.0f - (dist - 20.0f) / 6.0f));
                    Color outer = {100, 40, 10, alpha};
                    ImageDrawPixel(&img, x, y, outer);
                }
            }
        }
        
        for (int i = 0; i < 8; i++) {
            float angle = (float)i * PI * 2.0f / 8.0f + frame * 0.3f;
            int cx = 32 + (int)(cosf(angle) * 12.0f);
            int cy = 32 + (int)(sinf(angle) * 12.0f);
            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    if (dx*dx + dy*dy <= 4) {
                        int px = cx + dx;
                        int py = cy + dy;
                        if (px >= 0 && px < 64 && py >= 0 && py < 64) {
                            Color spark = {
                                (unsigned char)(255),
                                (unsigned char)(150 + frame * 20),
                                (unsigned char)(50),
                                200
                            };
                            ImageDrawPixel(&img, px, py, spark);
                        }
                    }
                }
            }
        }
        
        meteoriteTex[frame] = LoadTextureFromImage(img);
        meteoriteTexLoaded[frame] = meteoriteTex[frame].id > 0;
        UnloadImage(img);
        
        if (meteoriteTexLoaded[frame]) {
            SetTextureFilter(meteoriteTex[frame], TEXTURE_FILTER_POINT);
        }
    }
    
    TraceLog(LOG_INFO, "Loaded %d meteorite animation frames", METEORITE_FRAMES);
}

void World::UnloadMeteoriteSprites() {
    for (int i = 0; i < METEORITE_FRAMES; i++) {
        if (meteoriteTexLoaded[i]) {
            UnloadTexture(meteoriteTex[i]);
            meteoriteTexLoaded[i] = false;
        }
    }
}

void World::UpdateMeteorites(float dt) {
    // Update existing meteorites
    for (auto& meteorite : meteorites) {
        if (meteorite.active) {
            meteorite.Update(dt);
            
            // Check for collisions if not exploding
            if (!meteorite.exploding) {
                ProcessMeteoriteCollisions(meteorite);
            }
        }
    }
    
    // Remove inactive meteorites
    meteorites.erase(
        std::remove_if(meteorites.begin(), meteorites.end(),
            [](const Meteorite& m) { return m.ShouldRemove(); }),
        meteorites.end()
    );
}

void World::SpawnMeteorite(Vector2 targetPos) {
    // Random spawn position on any edge of screen
    int side = GetRandomValue(0, 3);
    Vector2 spawnPos;
    
    switch (side) {
        case 0: // Top
            spawnPos = {(float)GetRandomValue(50, worldW - 50), -50.0f};
            break;
        case 1: // Right
            spawnPos = {(float)worldW + 50.0f, (float)GetRandomValue(50, worldH - 50)};
            break;
        case 2: // Bottom
            spawnPos = {(float)GetRandomValue(50, worldW - 50), (float)worldH + 50.0f};
            break;
        case 3: // Left
            spawnPos = {-50.0f, (float)GetRandomValue(50, worldH - 50)};
            break;
        default:
            spawnPos = {(float)GetRandomValue(50, worldW - 50), -50.0f};
            break;
    }
    
    Meteorite meteorite(spawnPos, targetPos);
    meteorites.push_back(meteorite);
}

void World::ProcessMeteoriteCollisions(Meteorite& meteorite) {
    // Check if meteorite reached target position
    float distToTarget = Vector2Distance(meteorite.position, meteorite.targetPosition);
    bool reachedTarget = distToTarget < 50.0f;
    
    if (reachedTarget) {
        CreateCrater(meteorite.position, 80.0f);
        meteorite.StartExplosion();
        return;
    }
}

void World::CreateCrater(Vector2 worldPos, float radius) {
    // Store crater position for later regrowth (limit history size)
    if (craterHistory.size() < 1000) {
        craterHistory.push_back(worldPos);
    }
    
    // Convert world coordinates to tile coordinates
    int centerX = (int)(worldPos.x / CELL_SIZE);
    int centerY = (int)(worldPos.y / CELL_SIZE);
    int radiusTiles = (int)(radius / CELL_SIZE) + 1; // Add 1 for safety
    
    // Create crater in circular pattern
    for (int y = -radiusTiles; y <= radiusTiles; y++) {
        for (int x = -radiusTiles; x <= radiusTiles; x++) {
            // Check if within circular radius
            float distance = sqrtf(x * x + y * y);
            if (distance <= radiusTiles) {
                int tileX = centerX + x;
                int tileY = centerY + y;
                
                // Check bounds
                if (tileX >= 0 && tileX < cols && tileY >= 0 && tileY < rows) {
                    int tileIndex = tileY * cols + tileX;
                    
                    // Set terrain damage (create crater)
                    terrainDamage[tileIndex] = true;
                    
                    // Destroy settlement tiles in crater
                    for (auto& settlement : settlements) {
                        if (!settlement.alive) continue;
                        if (settlement.tiles.count(tileIndex) > 0) {
                            settlement.tiles.erase(tileIndex);
                        }
                    }
                    
                    // Kill any plants or animals in the crater
                    Vector2 tileWorldPos = {
                        (float)(tileX * CELL_SIZE + CELL_SIZE / 2),
                        (float)(tileY * CELL_SIZE + CELL_SIZE / 2)
                    };
                    
                    // Kill plants in crater
                    for (auto& plant : plants) {
                        float dist = Vector2Distance(plant.position, tileWorldPos);
                        if (dist < CELL_SIZE) {
                            plant.health = 0.0f;
                        }
                    }
                    
                    // Kill animals in crater
                    for (auto& animal : animals) {
                        float dist = Vector2Distance(animal.position, tileWorldPos);
                        if (dist < CELL_SIZE) {
                            animal.position = {-1000, -1000};
                        }
                    }
                    
                    // Kill NPCs in crater
                    for (auto& npc : npcs) {
                        if (!npc.alive || npc.isDying) continue;
                        float dist = Vector2Distance(npc.pos, tileWorldPos);
                        if (dist < CELL_SIZE * 1.5f) {
                            BeginNpcDeath(npc);
                        }
                    }
                    
                    // Destroy campfires in crater
                    for (auto& settlement : settlements) {
                        if (!settlement.alive) continue;
                        float dist = Vector2Distance(settlement.campfirePosPx, tileWorldPos);
                        if (dist < CELL_SIZE * 2.0f) {
                            settlement.campfirePosPx = {-1000, -1000};
                        }
                    }
                    
                    // Destroy barracks in crater
                    for (auto& settlement : settlements) {
                        if (!settlement.alive) continue;
                        for (int i = 0; i < (int)settlement.barracksList.size(); i++) {
                            if (!settlement.barracksList[i].alive) continue;
                            float dist = Vector2Distance(settlement.barracksList[i].posPx, tileWorldPos);
                            if (dist < CELL_SIZE * 3.0f) {
                                DamageSettlementBarracks((int)(&settlement - &settlements[0]), i, 10000.0f);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Check if crater is near water - if so, fill with water instead of healing
    for (int y = -radiusTiles - 2; y <= radiusTiles + 2; y++) {
        for (int x = -radiusTiles - 2; x <= radiusTiles + 2; x++) {
            int tileX = centerX + x;
            int tileY = centerY + y;
            
            if (tileX >= 0 && tileX < cols && tileY >= 0 && tileY < rows) {
                if (GetTileType(tileX, tileY) == TileType::SHALLOW_WATER) {
                    // Found water nearby - fill crater with water
                    for (int cy = -radiusTiles; cy <= radiusTiles; cy++) {
                        for (int cx = -radiusTiles; cx <= radiusTiles; cx++) {
                            float dist = sqrtf(cx * cx + cy * cy);
                            if (dist <= radiusTiles) {
                                int tx = centerX + cx;
                                int ty = centerY + cy;
                                if (tx >= 0 && tx < cols && ty >= 0 && ty < rows) {
                                    int tidx = ty * cols + tx;
                                    terrainDamage[tidx] = false; // Remove crater
                                    tileTypes[tidx] = TileType::SHALLOW_WATER; // Make water
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
    
    // Update settlement geometry after destroying tiles
    for (auto& settlement : settlements) {
        if (!settlement.alive) continue;
        if (settlement.tiles.empty()) {
            settlement.alive = false;
            if (settlement.warActive) {
                int sid = (int)(&settlement - &settlements[0]);
                StopSettlementWar(sid);
            }
        } else {
            settlement.centerPx = ComputeSettlementCenterPx(settlement);
            settlement.boundsPx = ComputeSettlementBoundsPx(settlement);
        }
    }
}

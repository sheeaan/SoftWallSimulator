#pragma once

#include "simulation/Enemy.hpp"
#include "simulation/Grid.hpp"
#include "simulation/Level.hpp"
#include "simulation/Pathfinder.hpp"
#include "simulation/Player.hpp"
#include "simulation/Raycaster.hpp"

#include <SFML/System/Vector2.hpp>

#include <vector>

// Numbers the statistics panel shows that the simulation cannot rebuild from its current state.
struct SimulationStatistics
{
    float visibilityMilliseconds = 0.f;  // The last 360-ray cast.
    float pathfindingMilliseconds = 0.f; // The last A* search.
    int pathRecalculations = 0;
    int wallsDestroyed = 0;
};

// The whole simulation. The presentation layer drives it through update() and reads it through const getters.
class World
{
public:
    explicit World(const Level& level);

    void update(float seconds, sf::Vector2f playerInputDirection);

    // Shoots the tile under a world position from the player's position. A soft wall loses one health
    // and gets a see-through slot along the line of fire. When it breaks, the enemy re-plans
    // inside this same call, so the new path exists before the next frame is drawn.
    DamageResult damageTileAt(sf::Vector2f worldPosition);

    const Grid& getGrid() const;
    const Player& getPlayer() const;
    const Enemy& getEnemy() const;
    const std::vector<RayHit>& getVisibility() const;
    const Pathfinder& getPathfinder() const;
    const SimulationStatistics& getStatistics() const;
    bool canPlayerSeeEnemy() const;

    // The full DDA record of one ray from the player toward a point, for the math view.
    RayTrace traceRayFromPlayer(sf::Vector2f toward) const;

private:
    void castVisibility();
    void recalculateEnemyPath();

    Grid grid;
    Player player;
    Enemy enemy;
    Raycaster raycaster;
    Pathfinder pathfinder;
    GridCell enemyPathTarget; // The player cell the enemy's current path leads to.
    SimulationStatistics statistics;
};

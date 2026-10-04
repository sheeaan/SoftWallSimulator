#pragma once

#include "simulation/Enemy.hpp"
#include "simulation/Grid.hpp"
#include "simulation/Level.hpp"
#include "simulation/Pathfinder.hpp"
#include "simulation/Player.hpp"
#include "simulation/Raycaster.hpp"

#include <SFML/System/Vector2.hpp>

#include <vector>

// The whole simulation. The presentation layer drives it through update() and reads it through const getters.
class World
{
public:
    explicit World(const Level& level);

    void update(float seconds, sf::Vector2f playerInputDirection);

    const Grid& getGrid() const;
    const Player& getPlayer() const;
    const Enemy& getEnemy() const;
    const std::vector<RayHit>& getVisibility() const;
    const Pathfinder& getPathfinder() const;
    bool canPlayerSeeEnemy() const;

private:
    void recalculateEnemyPath();

    Grid grid;
    Player player;
    Enemy enemy;
    Raycaster raycaster;
    Pathfinder pathfinder;
    GridCell enemyPathTarget; // The player cell the enemy's current path leads to.
};

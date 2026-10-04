#include "simulation/World.hpp"

World::World(const Level& level)
    : grid(level.grid)
    , player(cellCenter(level.playerSpawn))
    , enemy(cellCenter(level.enemySpawn))
{
    recalculateEnemyPath();
    raycaster.castVisibility(player.getPosition(), grid);
}

void World::update(float seconds, sf::Vector2f playerInputDirection)
{
    player.move(playerInputDirection, seconds, grid);

    // Within one cell the old path still leads to the player, so re-plan only when the player changes cell.
    if (player.getCell() != enemyPathTarget)
    {
        recalculateEnemyPath();
    }
    enemy.followPath(seconds);

    raycaster.castVisibility(player.getPosition(), grid);
}

const Grid& World::getGrid() const
{
    return grid;
}

const Player& World::getPlayer() const
{
    return player;
}

const Enemy& World::getEnemy() const
{
    return enemy;
}

const std::vector<RayHit>& World::getVisibility() const
{
    return raycaster.getVisibility();
}

const Pathfinder& World::getPathfinder() const
{
    return pathfinder;
}

bool World::canPlayerSeeEnemy() const
{
    return raycaster.hasLineOfSight(player.getPosition(), enemy.getPosition(), grid);
}

void World::recalculateEnemyPath()
{
    enemyPathTarget = player.getCell();
    enemy.setPath(pathfinder.findPath(grid, enemy.getCell(), enemyPathTarget));
}

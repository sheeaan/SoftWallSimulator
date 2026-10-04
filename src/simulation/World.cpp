#include "simulation/World.hpp"

#include <chrono>
#include <utility>

namespace
{
    float millisecondsSince(std::chrono::steady_clock::time_point start)
    {
        std::chrono::duration<float, std::milli> elapsed = std::chrono::steady_clock::now() - start;
        return elapsed.count();
    }
}

World::World(const Level& level)
    : grid(level.grid)
    , player(cellCenter(level.playerSpawn))
    , enemy(cellCenter(level.enemySpawn))
{
    recalculateEnemyPath();
    castVisibility();
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

    castVisibility();
}

DamageResult World::damageTileAt(sf::Vector2f worldPosition)
{
    DamageResult result = grid.damageTile(cellContaining(worldPosition));
    if (result == DamageResult::DESTROYED)
    {
        statistics.wallsDestroyed += 1;

        // The map's shape just changed, so the old path may be blocked or no longer the shortest.
        recalculateEnemyPath();
    }
    return result;
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

const SimulationStatistics& World::getStatistics() const
{
    return statistics;
}

bool World::canPlayerSeeEnemy() const
{
    return raycaster.hasLineOfSight(player.getPosition(), enemy.getPosition(), grid);
}

void World::castVisibility()
{
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    raycaster.castVisibility(player.getPosition(), grid);
    statistics.visibilityMilliseconds = millisecondsSince(start);
}

void World::recalculateEnemyPath()
{
    enemyPathTarget = player.getCell();

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::vector<GridCell> path = pathfinder.findPath(grid, enemy.getCell(), enemyPathTarget);
    statistics.pathfindingMilliseconds = millisecondsSince(start);
    statistics.pathRecalculations += 1;

    enemy.setPath(std::move(path));
}

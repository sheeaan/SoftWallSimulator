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
    // The shot flies from the player to the clicked point and drills its slot through the wall there.
    GridCell cell = cellContaining(worldPosition);
    std::uint16_t panelsHit = panelsAlongShot(cell, player.getPosition(), worldPosition);
    DamageResult result = grid.damageTile(cell, panelsHit);
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

RayTrace World::traceRayFromPlayer(sf::Vector2f toward) const
{
    sf::Vector2f offset = toward - player.getPosition();
    sf::Vector2f direction(1.f, 0.f); // A point on top of the player has no direction; look east.
    if (offset.length() > 0.001f)
    {
        direction = offset.normalized();
    }
    return raycaster.traceRay(player.getPosition(), direction, grid);
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

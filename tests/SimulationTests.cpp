// Framework-free tests for the simulation layer. Each test prints PASS or FAIL.
// The process exits with 1 if any test fails, so a build script can check it.

#include "simulation/Grid.hpp"
#include "simulation/Level.hpp"
#include "simulation/Pathfinder.hpp"
#include "simulation/Raycaster.hpp"
#include "simulation/SimulatorErrors.hpp"
#include "simulation/World.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    using TestFunction = bool (*)();

    int passedTestCount = 0;
    int failedTestCount = 0;

    void runTest(const std::string& testName, TestFunction test)
    {
        bool passed = false;
        std::string failureReason;
        try
        {
            passed = test();
        }
        catch (const SimulatorError& error)
        {
            failureReason = std::string(" (threw: ") + error.what() + ")";
        }

        if (passed)
        {
            std::cout << "PASS  " << testName << '\n';
            passedTestCount += 1;
        }
        else
        {
            std::cout << "FAIL  " << testName << failureReason << '\n';
            failedTestCount += 1;
        }
    }

    bool isNear(float actual, float expected)
    {
        return std::abs(actual - expected) < 0.0001f;
    }

    int pathCost(const std::vector<GridCell>& path)
    {
        int cost = 0;
        for (std::size_t index = 1; index < path.size(); ++index)
        {
            bool movedColumn = path[index].column != path[index - 1].column;
            bool movedRow = path[index].row != path[index - 1].row;
            if (movedColumn && movedRow)
            {
                cost += Pathfinder::DiagonalStepCost;
            }
            else
            {
                cost += Pathfinder::StraightStepCost;
            }
        }
        return cost;
    }

    bool pathContains(const std::vector<GridCell>& path, GridCell cell)
    {
        for (const GridCell& pathCell : path)
        {
            if (pathCell == cell)
            {
                return true;
            }
        }
        return false;
    }

    // Player and enemy in one corridor, with a soft wall at column 3 between them.
    const std::vector<std::string> CorridorRows = {
        "#######",
        "#P.+.E#",
        "#######",
    };
    const sf::Vector2f CorridorSoftWallPosition(3.5f, 1.5f);

    // ---- Grid ----

    bool gridStoresCellsInRowMajorOrder()
    {
        Grid grid(10, 4);
        bool holdsColumnsTimesRows = grid.getCellCount() == 40;
        bool indexIsRowTimesWidthPlusColumn = grid.indexOf(GridCell{3, 2}) == 23;
        bool indexMapsBackToCell = grid.cellAt(23) == GridCell{3, 2};

        grid.setTile(GridCell{3, 2}, TileType::HARD_WALL);
        bool tileIsStoredAtThatCell = grid.getTile(GridCell{3, 2}).type == TileType::HARD_WALL;

        return holdsColumnsTimesRows && indexIsRowTimesWidthPlusColumn && indexMapsBackToCell
               && tileIsStoredAtThatCell;
    }

    // ---- Destruction ----

    bool clickPositionMapsToCellUnderIt()
    {
        // Game turns the mouse pixel into a world position first; this is the world-to-cell step.
        bool insideCell = cellContaining(sf::Vector2f(3.99f, 0.01f)) == GridCell{3, 0};
        bool boundaryBelongsToNextCell = cellContaining(sf::Vector2f(4.f, 2.5f)) == GridCell{4, 2};
        bool leftOfMapStaysOutside = cellContaining(sf::Vector2f(-0.25f, 1.5f)) == GridCell{-1, 1};
        return insideCell && boundaryBelongsToNextCell && leftOfMapStaysOutside;
    }

    bool clickingSoftWallReducesHealthByOne()
    {
        World world(parseLevel(CorridorRows));
        DamageResult result = world.damageTileAt(CorridorSoftWallPosition);

        const Tile& tile = world.getGrid().getTile(GridCell{3, 1});
        bool reportedDamage = result == DamageResult::DAMAGED;
        bool lostOneHealth = tile.health == Grid::SoftWallMaxHealth - 1;
        bool stillStanding = tile.type == TileType::SOFT_WALL;
        return reportedDamage && lostOneHealth && stillStanding;
    }

    bool softWallBecomesEmptyAtZeroHealth()
    {
        World world(parseLevel(CorridorRows));
        DamageResult lastResult = DamageResult::NO_EFFECT;
        for (int hit = 0; hit < Grid::SoftWallMaxHealth; ++hit)
        {
            lastResult = world.damageTileAt(CorridorSoftWallPosition);
        }

        bool lastHitDestroyed = lastResult == DamageResult::DESTROYED;
        bool tileIsEmpty = world.getGrid().getTile(GridCell{3, 1}).type == TileType::EMPTY;
        return lastHitDestroyed && tileIsEmpty;
    }

    // ---- Vision ----

    bool visionCastsExactly360Rays()
    {
        Grid grid(9, 9);
        Raycaster raycaster;
        raycaster.castVisibility(sf::Vector2f(4.5f, 4.5f), grid);
        return raycaster.getVisibility().size() == 360;
    }

    bool rayStopsAtFirstNonEmptyTile()
    {
        // A soft wall at column 5 hides a hard wall at column 7 behind it.
        Grid grid(10, 5);
        grid.setTile(GridCell{5, 2}, TileType::SOFT_WALL);
        grid.setTile(GridCell{7, 2}, TileType::HARD_WALL);

        Raycaster raycaster;
        RayHit hit = raycaster.castRay(sf::Vector2f(2.5f, 2.5f), sf::Vector2f(1.f, 0.f), grid);

        bool stoppedAtSoftWall = hit.cell == GridCell{5, 2};
        bool distanceReachesWallFace = isNear(hit.distance, 2.5f);
        bool pointIsOnWallFace = isNear(hit.point.x, 5.f) && isNear(hit.point.y, 2.5f);
        return stoppedAtSoftWall && distanceReachesWallFace && pointIsOnWallFace;
    }

    // ---- A* ----

    bool aStarFindsShortestPathAroundWall()
    {
        // The wall forces a detour over the top or the bottom. With 8-way moves and no corner cutting,
        // the cheapest detour is two diagonal steps and four straight steps: 2 x 14 + 4 x 10 = 68.
        Level level = parseLevel({
            ".......",
            "...#...",
            ".P.#.E.",
            "...#...",
            ".......",
        });
        Pathfinder pathfinder;
        std::vector<GridCell> path = pathfinder.findPath(level.grid, level.playerSpawn, level.enemySpawn);

        bool startsAtStart = !path.empty() && path.front() == level.playerSpawn;
        bool endsAtGoal = !path.empty() && path.back() == level.enemySpawn;
        return startsAtStart && endsAtGoal && pathCost(path) == 68;
    }

    bool destroyingWallRecalculatesEnemyPathInSameCall()
    {
        // The soft wall seals the enemy off, so it starts with no path at all.
        World world(parseLevel(CorridorRows));
        bool startsWithoutPath = world.getEnemy().getPath().empty();

        for (int hit = 0; hit < Grid::SoftWallMaxHealth; ++hit)
        {
            world.damageTileAt(CorridorSoftWallPosition);
        }

        // No update() in between: the path must already exist when damageTileAt returns.
        const std::vector<GridCell>& path = world.getEnemy().getPath();
        bool pathNowExists = !path.empty();
        bool pathRunsThroughOpening = pathContains(path, GridCell{3, 1});
        bool pathEndsAtPlayer = pathNowExists && path.back() == world.getPlayer().getCell();
        return startsWithoutPath && pathNowExists && pathRunsThroughOpening && pathEndsAtPlayer;
    }

    // ---- End to end ----

    bool endToEndBreakingWallOpensShortcutEnemyWalksThrough()
    {
        // The top row is the short way, but a soft wall blocks it. The enemy starts on the long way around.
        World world(parseLevel({
            "#########",
            "#P..+..E#",
            "#.#####.#",
            "#.......#",
            "#########",
        }));
        int detourCost = pathCost(world.getEnemy().getPath());

        DamageResult firstHit = world.damageTileAt(sf::Vector2f(4.5f, 1.5f));
        DamageResult secondHit = world.damageTileAt(sf::Vector2f(4.5f, 1.5f));
        DamageResult thirdHit = world.damageTileAt(sf::Vector2f(4.5f, 1.5f));
        bool hitsBreakTheWall = firstHit == DamageResult::DAMAGED && secondHit == DamageResult::DAMAGED
                                && thirdHit == DamageResult::DESTROYED;

        int shortcutCost = pathCost(world.getEnemy().getPath());
        bool shortcutIsCheaper = detourCost == 100 && shortcutCost == 60;

        // Run two simulated seconds with the player standing still, and watch the enemy's cell.
        bool enemyWalkedThroughOpening = false;
        for (int frame = 0; frame < 20; ++frame)
        {
            world.update(0.1f, sf::Vector2f(0.f, 0.f));
            if (world.getEnemy().getCell() == GridCell{4, 1})
            {
                enemyWalkedThroughOpening = true;
            }
        }
        bool enemyReachedPlayer = world.getEnemy().getCell() == world.getPlayer().getCell();

        return hitsBreakTheWall && shortcutIsCheaper && enemyWalkedThroughOpening && enemyReachedPlayer;
    }

    bool endToEndBreakingWallGivesLineOfSight()
    {
        World world(parseLevel(CorridorRows));
        bool hiddenAtStart = !world.canPlayerSeeEnemy();
        float eastRayBefore = world.getVisibility()[0].distance; // Ray 0 points east, along the corridor.

        for (int hit = 0; hit < Grid::SoftWallMaxHealth; ++hit)
        {
            world.damageTileAt(CorridorSoftWallPosition);
        }
        world.update(0.f, sf::Vector2f(0.f, 0.f));

        bool visibleAfterBreak = world.canPlayerSeeEnemy();
        float eastRayAfter = world.getVisibility()[0].distance;
        bool rayStoppedAtSoftWallBefore = isNear(eastRayBefore, 1.5f);
        bool rayReachesFarWallAfter = isNear(eastRayAfter, 4.5f);
        return hiddenAtStart && visibleAfterBreak && rayStoppedAtSoftWallBefore && rayReachesFarWallAfter;
    }
}

int main()
{
    std::cout << "Soft Wall Simulator - simulation tests\n\n";

    runTest("Grid stores cells in row-major order in one vector", gridStoresCellsInRowMajorOrder);
    runTest("A click position maps to the cell under it", clickPositionMapsToCellUnderIt);
    runTest("Clicking a soft wall reduces its health by one", clickingSoftWallReducesHealthByOne);
    runTest("A soft wall becomes EMPTY at zero health", softWallBecomesEmptyAtZeroHealth);
    runTest("Vision casts exactly 360 rays", visionCastsExactly360Rays);
    runTest("A ray stops at the first non-EMPTY tile", rayStopsAtFirstNonEmptyTile);
    runTest("A* finds the shortest path around a wall", aStarFindsShortestPathAroundWall);
    runTest("Destroying a wall recalculates the enemy path within the same call",
            destroyingWallRecalculatesEnemyPathInSameCall);
    runTest("End to end: breaking a soft wall opens a shortcut, and the enemy walks through it",
            endToEndBreakingWallOpensShortcutEnemyWalksThrough);
    runTest("End to end: breaking the wall between player and enemy gives line of sight",
            endToEndBreakingWallGivesLineOfSight);

    std::cout << '\n' << passedTestCount << " passed, " << failedTestCount << " failed\n";
    if (failedTestCount > 0)
    {
        return 1;
    }
    return 0;
}

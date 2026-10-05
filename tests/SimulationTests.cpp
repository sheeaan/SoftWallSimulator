// Framework-free tests for the simulation layer. Each test prints PASS or FAIL.
// The process exits with 1 if any test fails, so a build script can check it.

#include "simulation/Grid.hpp"
#include "simulation/Level.hpp"
#include "simulation/Pathfinder.hpp"
#include "simulation/Raycaster.hpp"
#include "simulation/SimulatorErrors.hpp"
#include "simulation/World.hpp"

#include <cmath>
#include <cstddef>
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

    // Advances the world in 0.05-second steps, the same cap the game puts on a frame, so nothing tunnels through a wall.
    void runWorld(World& world, float seconds, sf::Vector2f inputDirection)
    {
        const float stepSeconds = 0.05f;
        long stepCount = std::lround(seconds / stepSeconds);
        for (long stepIndex = 0; stepIndex < stepCount; ++stepIndex)
        {
            world.update(stepSeconds, inputDirection);
        }
    }

    TileType tileAt(const World& world, GridCell cell)
    {
        return world.getGrid().getTile(cell).type;
    }

    const sf::Vector2f NoInput(0.f, 0.f);

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

    bool shotOpensHoleThatVisionPassesThrough()
    {
        // A soft wall at column 5, with a hard wall behind it at column 8.
        Grid grid(10, 5);
        grid.setTile(GridCell{5, 2}, TileType::SOFT_WALL);
        grid.setTile(GridCell{8, 2}, TileType::HARD_WALL);

        // A horizontal shot at height 2.5 drills the slot through panel row 2, which spans 2.5 to 2.75.
        std::uint16_t slot = panelsAlongShot(GridCell{5, 2}, sf::Vector2f(2.5f, 2.5f), sf::Vector2f(5.1f, 2.5f));
        grid.damageTile(GridCell{5, 2}, slot);

        Raycaster raycaster;
        RayHit throughSlot = raycaster.castRay(sf::Vector2f(2.5f, 2.5f), sf::Vector2f(1.f, 0.f), grid);
        RayHit besideSlot = raycaster.castRay(sf::Vector2f(2.5f, 2.1f), sf::Vector2f(1.f, 0.f), grid);

        bool wallStillStands = grid.getTile(GridCell{5, 2}).type == TileType::SOFT_WALL;
        bool rayPassesThroughSlot = throughSlot.cell == GridCell{8, 2} && isNear(throughSlot.distance, 5.5f);
        bool rayBesideSlotStops = besideSlot.cell == GridCell{5, 2} && isNear(besideSlot.distance, 2.5f);
        return wallStillStands && rayPassesThroughSlot && rayBesideSlotStops;
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

    bool endToEndShootingHoleShowsEnemyThroughStandingWall()
    {
        World world(parseLevel(CorridorRows));
        bool hiddenAtStart = !world.canPlayerSeeEnemy();

        // One shot from the player, straight along the corridor.
        DamageResult result = world.damageTileAt(CorridorSoftWallPosition);

        bool wallStillStands = result == DamageResult::DAMAGED
                               && world.getGrid().getTile(GridCell{3, 1}).type == TileType::SOFT_WALL;
        bool seesEnemyThroughHole = world.canPlayerSeeEnemy();
        bool enemyStillHasNoRoute = world.getEnemy().getPath().empty(); // A hole does not make a wall walkable.
        return hiddenAtStart && wallStillStands && seesEnemyThroughHole && enemyStillHasNoRoute;
    }

    bool endToEndHoleGivesSightOnlyAlongLineOfFire()
    {
        // A full soft-wall divider between player and enemy.
        World world(parseLevel({
            "#########",
            "#...+...#",
            "#P..+..E#",
            "#...+...#",
            "#########",
        }));

        // Shoot the middle of the divider from the spawn, then step 0.6 tiles up, out of line with the slot.
        world.damageTileAt(sf::Vector2f(4.5f, 2.5f));
        bool seesFromWhereItShot = world.canPlayerSeeEnemy();

        world.update(0.1f, sf::Vector2f(0.f, -1.f));
        bool playerMovedUp = world.getPlayer().getPosition().y < 2.f;
        bool lostSightAfterMoving = !world.canPlayerSeeEnemy();
        return seesFromWhereItShot && playerMovedUp && lostSightAfterMoving;
    }

    // ---- Math views ----

    bool ddaTraceRecordsEveryStep()
    {
        // A ray from (0.5, 0.5) along (2, 1) meets a hard wall at (3, 1). Worked by hand:
        // tDeltaX = 1 / |dx| = 1.118 and tDeltaY = 1 / |dy| = 2.236. The first boundaries are at
        // tMaxX = 0.5 x 1.118 = 0.559 and tMaxY = 0.5 x 2.236 = 1.118.
        // Steps: column to (1, 0) at 0.559, row to (1, 1) at 1.118, column to (2, 1) at 1.677,
        // column to (3, 1) at 2.795, where the wall stops the ray on its face at (3.0, 1.75).
        Grid grid(6, 4);
        grid.setTile(GridCell{3, 1}, TileType::HARD_WALL);
        Raycaster raycaster;
        RayTrace trace = raycaster.traceRay(sf::Vector2f(0.5f, 0.5f), sf::Vector2f(2.f, 1.f).normalized(), grid);
        if (trace.steps.size() != 4)
        {
            return false;
        }

        bool deltasMatch = isNear(trace.distancePerColumn, 1.118034f) && isNear(trace.distancePerRow, 2.236068f);
        bool firstBoundariesMatch = isNear(trace.steps[0].distanceToNextColumn, 0.559017f)
                                    && isNear(trace.steps[0].distanceToNextRow, 1.118034f);
        bool axesMatch = trace.steps[0].crossedColumn && !trace.steps[1].crossedColumn && trace.steps[2].crossedColumn
                         && trace.steps[3].crossedColumn;
        bool cellsMatch = trace.steps[0].cell == GridCell{1, 0} && trace.steps[1].cell == GridCell{1, 1}
                          && trace.steps[2].cell == GridCell{2, 1} && trace.steps[3].cell == GridCell{3, 1};
        bool crossingsMatch = isNear(trace.steps[0].crossingDistance, 0.559017f)
                              && isNear(trace.steps[1].crossingDistance, 1.118034f)
                              && isNear(trace.steps[2].crossingDistance, 1.677051f)
                              && isNear(trace.steps[3].crossingDistance, 2.795085f);
        bool hitOnWallFace = isNear(trace.hit.point.x, 3.f) && isNear(trace.hit.point.y, 1.75f);
        return deltasMatch && firstBoundariesMatch && axesMatch && cellsMatch && crossingsMatch && hitOnWallFace;
    }

    bool aStarKeepsCostsAndParentsOfExpandedCells()
    {
        Level level = parseLevel({
            ".......",
            "...#...",
            ".P.#.E.",
            "...#...",
            ".......",
        });
        Pathfinder pathfinder;
        std::vector<GridCell> path = pathfinder.findPath(level.grid, level.playerSpawn, level.enemySpawn);
        std::size_t startIndex = level.grid.indexOf(level.playerSpawn);
        std::size_t goalIndex = level.grid.indexOf(level.enemySpawn);

        bool goalWasExpanded = pathfinder.wasExpanded(goalIndex);
        bool startCostsNothing = pathfinder.getCostFromStart(startIndex) == 0
                                 && !pathfinder.getCameFrom(startIndex).has_value();
        bool goalCostIsPathCost = pathfinder.getCostFromStart(goalIndex) == pathCost(path);
        bool estimatesAreOctile = Pathfinder::estimateCost(level.playerSpawn, level.enemySpawn) == 40
                                  && Pathfinder::estimateCost(level.enemySpawn, level.enemySpawn) == 0;

        // Following parents from the goal walks the path backwards to the start.
        std::size_t cellIndex = goalIndex;
        std::size_t parentsFollowed = 0;
        while (pathfinder.getCameFrom(cellIndex).has_value() && parentsFollowed < path.size())
        {
            cellIndex = pathfinder.getCameFrom(cellIndex).value();
            parentsFollowed += 1;
        }
        bool parentsLeadToStart = cellIndex == startIndex && parentsFollowed + 1 == path.size();

        return goalWasExpanded && startCostsNothing && goalCostIsPathCost && estimatesAreOctile && parentsLeadToStart;
    }

    bool tilesSitContiguouslyInMemory()
    {
        // Cell (12, 7) has index 7 x 48 + 12 = 348. At 4 bytes per tile, it starts 1392 bytes after cell (0, 0).
        Grid grid(48, 27);
        const Tile* firstTile = &grid.getTile(GridCell{0, 0});
        const Tile* cellTile = &grid.getTile(GridCell{12, 7});
        std::ptrdiff_t tilesApart = cellTile - firstTile;

        bool fourBytesPerTile = sizeof(Tile) == 4;
        bool tilesApartIsIndex = tilesApart == 348 && grid.indexOf(GridCell{12, 7}) == 348;
        bool bytesApartIsIndexTimesSize = static_cast<std::size_t>(tilesApart) * sizeof(Tile) == 1392;
        return fourBytesPerTile && tilesApartIsIndex && bytesApartIsIndexTimesSize;
    }

    bool endToEndDdaTraceAgreesWithLineOfSight()
    {
        World world(parseLevel(CorridorRows));
        sf::Vector2f enemyPosition = world.getEnemy().getPosition();

        // Before the shot, the trace ends on the soft wall, and the enemy is hidden.
        RayTrace before = world.traceRayFromPlayer(enemyPosition);
        bool stopsAtWall = !before.steps.empty() && before.steps.back().cell == GridCell{3, 1}
                           && isNear(before.hit.distance, 1.5f);
        bool hiddenBefore = !world.canPlayerSeeEnemy();

        // After one shot along the corridor, the trace runs through the slot to the far wall.
        world.damageTileAt(CorridorSoftWallPosition);
        RayTrace after = world.traceRayFromPlayer(enemyPosition);
        bool runsThroughSlot = !after.steps.empty() && after.steps.back().cell == GridCell{6, 1}
                               && isNear(after.hit.distance, 4.5f);
        bool visibleAfter = world.canPlayerSeeEnemy();

        return stopsAtWall && hiddenBefore && runsThroughSlot && visibleAfter;
    }

    bool endToEndRouteCostsAddUpStepByStep()
    {
        // The detour from the shortcut test: 100 is the cheapest route.
        World world(parseLevel({
            "#########",
            "#P..+..E#",
            "#.#####.#",
            "#.......#",
            "#########",
        }));
        const Grid& grid = world.getGrid();
        const Pathfinder& pathfinder = world.getPathfinder();
        const std::vector<GridCell>& path = world.getEnemy().getPath();
        if (path.size() < 2)
        {
            return false;
        }

        // Each step along the route adds exactly its own cost to g.
        bool everyStepAddsItsCost = true;
        for (std::size_t index = 1; index < path.size(); ++index)
        {
            int costBefore = pathfinder.getCostFromStart(grid.indexOf(path[index - 1]));
            int costAfter = pathfinder.getCostFromStart(grid.indexOf(path[index]));
            bool diagonal = path[index].column != path[index - 1].column && path[index].row != path[index - 1].row;
            int expectedStepCost = Pathfinder::StraightStepCost;
            if (diagonal)
            {
                expectedStepCost = Pathfinder::DiagonalStepCost;
            }
            if (costAfter - costBefore != expectedStepCost)
            {
                everyStepAddsItsCost = false;
            }
        }

        bool goalCostIsRouteCost = pathfinder.getCostFromStart(grid.indexOf(path.back())) == 100;
        bool estimateNeverOverestimates = Pathfinder::estimateCost(pathfinder.getStart(), pathfinder.getGoal()) <= 100;
        return everyStepAddsItsCost && goalCostIsRouteCost && estimateNeverOverestimates;
    }

    // ---- Siege mechanics ----

    bool reinforcingTurnsDrywallIntoSteel()
    {
        Grid grid(5, 3);
        grid.setTile(GridCell{2, 1}, TileType::SOFT_WALL);
        grid.setTile(GridCell{4, 1}, TileType::HARD_WALL);

        bool reinforced = grid.reinforceTile(GridCell{2, 1});
        bool isSteel = grid.getTile(GridCell{2, 1}).type == TileType::REINFORCED_WALL;
        bool blocksWalkingAndVision = !grid.isWalkable(GridCell{2, 1}) && grid.blocksVision(GridCell{2, 1});
        bool concreteCannotBeReinforced = !grid.reinforceTile(GridCell{4, 1});
        return reinforced && isSteel && blocksWalkingAndVision && concreteCannotBeReinforced;
    }

    bool bulletsCannotOpenReinforcedWall()
    {
        World world(parseLevel({
            "#######",
            "#P.R.E#",
            "#######",
        }));
        bool everyShotBounces = true;
        for (int shot = 0; shot < 5; ++shot)
        {
            if (world.damageTileAt(sf::Vector2f(3.5f, 1.5f)) != DamageResult::NO_EFFECT)
            {
                everyShotBounces = false;
            }
        }
        bool stillSteel = tileAt(world, GridCell{3, 1}) == TileType::REINFORCED_WALL;
        return everyShotBounces && stillSteel && !world.canPlayerSeeEnemy();
    }

    bool hardBreachChargeBurnsThroughAfterFuse()
    {
        // The attacker stands right next to the reinforced wall.
        World world(parseLevel({
            "#######",
            "#.PR.E#",
            "#######",
        }));
        ChargePlacement placement = world.placeCharge(ChargeType::HARD_BREACH, sf::Vector2f(3.5f, 1.5f));
        bool placed = placement == ChargePlacement::PLACED
                      && world.getChargesLeft(ChargeType::HARD_BREACH) == World::HardBreachChargeCount - 1;

        world.update(2.9f, NoInput);
        bool holdsDuringFuse = tileAt(world, GridCell{3, 1}) == TileType::REINFORCED_WALL;
        world.update(0.2f, NoInput);
        bool burnedThrough = tileAt(world, GridCell{3, 1}) == TileType::EMPTY && world.getDetonations().size() == 1;
        return placed && holdsDuringFuse && burnedThrough;
    }

    bool breachChargeOpensSoftWallAndSoftNeighbors()
    {
        // Above the target is drywall; below it is steel.
        World world(parseLevel({
            "#######",
            "#..+..#",
            "#.P+.E#",
            "#..R..#",
            "#######",
        }));
        bool steelRejected = world.placeCharge(ChargeType::BREACH, sf::Vector2f(3.5f, 3.5f)) == ChargePlacement::WRONG_SURFACE;
        bool placed = world.placeCharge(ChargeType::BREACH, sf::Vector2f(3.5f, 2.5f)) == ChargePlacement::PLACED;

        runWorld(world, World::BreachFuseSeconds + 0.1f, NoInput);
        bool targetOpen = tileAt(world, GridCell{3, 2}) == TileType::EMPTY;
        bool softNeighborOpen = tileAt(world, GridCell{3, 1}) == TileType::EMPTY;
        bool steelNeighborHolds = tileAt(world, GridCell{3, 3}) == TileType::REINFORCED_WALL;
        return steelRejected && placed && targetOpen && softNeighborOpen && steelNeighborHolds;
    }

    bool barricadeBlocksDoorwayAndBreaksInOneShot()
    {
        World world(parseLevel({
            "#######",
            "#P.=.E#",
            "#######",
        }));
        bool blocksWalking = !world.getGrid().isWalkable(GridCell{3, 1}) && world.getEnemy().getPath().empty();
        bool blocksSight = !world.canPlayerSeeEnemy();

        DamageResult result = world.damageTileAt(sf::Vector2f(3.5f, 1.5f));
        bool brokeInOneShot = result == DamageResult::DESTROYED && tileAt(world, GridCell{3, 1}) == TileType::EMPTY;
        return blocksWalking && blocksSight && brokeInOneShot && world.canPlayerSeeEnemy();
    }

    bool droneIsFastAndCastsItsOwnRays()
    {
        World world(parseLevel({
            "##########",
            "#........#",
            "#.P......#",
            "#........#",
            "#.......E#",
            "##########",
        }));
        sf::Vector2f playerStart = world.getPlayer().getPosition();
        world.toggleDrone();
        bool driving = world.isDrivingDrone() && world.getDrone().has_value();

        runWorld(world, 0.2f, sf::Vector2f(1.f, 0.f));
        float droneTravel = world.getDrone().value().getPosition().x - playerStart.x;
        bool movedAtDroneSpeed = isNear(droneTravel, Drone::Speed * 0.2f);
        bool playerStayed = world.getPlayer().getPosition() == playerStart;
        bool fasterThanPlayer = Drone::Speed > Player::Speed;
        bool castsOwnRays = world.getDroneVisibility().size() == 360;
        return driving && movedAtDroneSpeed && playerStayed && fasterThanPlayer && castsOwnRays;
    }

    bool droneSpotsEnemyAndMarkFades()
    {
        // The defender is behind a wall. The drone rolls under the barricade to see it.
        World world(parseLevel({
            "#########",
            "#P..#..E#",
            "#...=...#",
            "#...#...#",
            "#########",
        }));
        bool hiddenAtStart = !world.isEnemySpotted();

        world.toggleDrone();
        runWorld(world, 0.1f, sf::Vector2f(0.f, 1.f));
        runWorld(world, 0.45f, sf::Vector2f(1.f, 0.f));
        bool droneRolledUnderBarricade = world.getDrone().value().getCell() == GridCell{5, 2};
        bool spottedByDrone = world.isEnemySpotted();

        // Back to the attacker, who cannot see the defender: the mark lasts four seconds.
        world.toggleDrone();
        runWorld(world, 3.9f, NoInput);
        bool stillMarked = world.isEnemySpotted();
        runWorld(world, 0.2f, NoInput);
        bool markFaded = !world.isEnemySpotted();
        return hiddenAtStart && droneRolledUnderBarricade && spottedByDrone && stillMarked && markFaded;
    }

    bool holdingBombSitePlantsDefuserAndWins()
    {
        Round round(true);
        round.update(Round::PreparationSeconds + 0.01f, false);
        bool actionStarted = round.getPhase() == RoundPhase::ACTION;

        // Stepping out of the site starts the plant over.
        round.update(2.f, true);
        round.update(0.1f, false);
        bool leavingResetsPlant = round.getPlantProgress() == 0.f;

        for (int step = 0; step < 41; ++step)
        {
            round.update(0.1f, true);
        }
        bool attackersWon = round.getPhase() == RoundPhase::ATTACKERS_WIN
                            && round.getEndReason() == RoundEndReason::DEFUSER_PLANTED;
        return actionStarted && leavingResetsPlant && attackersWon;
    }

    bool defenderReinforcesPlannedWallsDuringPreparation()
    {
        World world(parseLevel({
            "##########",
            "#P...r.G.#",
            "#....r.*E#",
            "##########",
        }));
        bool startsPreparing = world.getRound().getPhase() == RoundPhase::PREPARATION
                               && world.getDefenderState() == DefenderState::PREPARING;

        runWorld(world, 6.f, NoInput);
        bool bothReinforced = tileAt(world, GridCell{5, 1}) == TileType::REINFORCED_WALL
                              && tileAt(world, GridCell{5, 2}) == TileType::REINFORCED_WALL;
        bool stillPreparationPhase = world.getRound().getPhase() == RoundPhase::PREPARATION;
        bool nowGuarding = world.getDefenderState() == DefenderState::GUARDING;
        return startsPreparing && bothReinforced && stillPreparationPhase && nowGuarding;
    }

    bool runningOutOfTimeMeansDefendersWin()
    {
        Round round(true);
        round.update(Round::PreparationSeconds + 0.01f, false);
        round.update(Round::ActionSeconds - 1.f, false);
        bool stillPlaying = round.getPhase() == RoundPhase::ACTION;
        round.update(1.5f, false);
        bool defendersWon = round.getPhase() == RoundPhase::DEFENDERS_WIN
                            && round.getEndReason() == RoundEndReason::TIME_EXPIRED;
        return stillPlaying && defendersWon;
    }

    bool defenderReachingAttackerWinsRound()
    {
        // One open room: the moment the action phase starts, the defender sees the attacker.
        World world(parseLevel({
            "##########",
            "#P.....G*#",
            "#......*E#",
            "##########",
        }));
        runWorld(world, Round::PreparationSeconds + 5.f, NoInput);
        return world.getRound().getPhase() == RoundPhase::DEFENDERS_WIN
               && world.getRound().getEndReason() == RoundEndReason::ATTACKER_ELIMINATED;
    }

    bool defenderGuardsUntilItSpotsAttacker()
    {
        // The wall hides the attacker's corner from the guard point. Walking right along the bottom row reveals them.
        World world(parseLevel({
            "############",
            "#....#...G*#",
            "#....#....*#",
            "#..P......E#",
            "############",
        }));
        runWorld(world, Round::PreparationSeconds + 1.f, NoInput);
        bool guardsWhileHidden = world.getDefenderState() == DefenderState::GUARDING;

        runWorld(world, 0.5f, sf::Vector2f(1.f, 0.f));
        bool huntsOnceSeen = world.getDefenderState() == DefenderState::HUNTING;
        return guardsWhileHidden && huntsOnceSeen;
    }

    bool endToEndDefenderReinforcesAttackerBreachesDefenderHunts()
    {
        World world(parseLevel({
            "###########",
            "#....r.G*.#",
            "#.P..r.**.#",
            "#....r.**E#",
            "###########",
        }));

        // Preparation: the defender walls off the site with steel.
        runWorld(world, Round::PreparationSeconds + 0.1f, NoInput);
        bool wallReinforced = tileAt(world, GridCell{5, 1}) == TileType::REINFORCED_WALL
                              && tileAt(world, GridCell{5, 2}) == TileType::REINFORCED_WALL
                              && tileAt(world, GridCell{5, 3}) == TileType::REINFORCED_WALL;

        // Action: walk up to the steel and burn through it.
        runWorld(world, 0.35f, sf::Vector2f(1.f, 0.f));
        bool chargePlaced = world.placeCharge(ChargeType::HARD_BREACH, sf::Vector2f(5.5f, 2.5f)) == ChargePlacement::PLACED;
        bool guardingBehindSteel = world.getDefenderState() == DefenderState::GUARDING;
        runWorld(world, World::HardBreachFuseSeconds + 0.05f, NoInput);
        bool breachOpen = tileAt(world, GridCell{5, 1}) == TileType::EMPTY && tileAt(world, GridCell{5, 2}) == TileType::EMPTY
                          && tileAt(world, GridCell{5, 3}) == TileType::EMPTY;

        // The defender sees the attacker through the breach and comes through it.
        bool hunting = world.getDefenderState() == DefenderState::HUNTING;
        const std::vector<GridCell>& path = world.getEnemy().getPath();
        bool routeThroughBreach = pathContains(path, GridCell{5, 1}) || pathContains(path, GridCell{5, 2})
                                  || pathContains(path, GridCell{5, 3});
        runWorld(world, 2.f, NoInput);
        bool attackerCaught = world.getRound().getEndReason() == RoundEndReason::ATTACKER_ELIMINATED;

        return wallReinforced && chargePlaced && guardingBehindSteel && breachOpen && hunting && routeThroughBreach
               && attackerCaught;
    }

    bool endToEndAttackerBreaksBarricadeAndPlants()
    {
        // The defender's room only touches the site through steel, so the defender cannot reach it.
        World world(parseLevel({
            "###########",
            "#P..=..G..#",
            "#...#..*..#",
            "#...####R##",
            "#...#....E#",
            "###########",
        }));
        runWorld(world, Round::PreparationSeconds + 0.05f, NoInput);
        bool actionPhase = world.getRound().getPhase() == RoundPhase::ACTION;

        runWorld(world, 0.25f, sf::Vector2f(1.f, 0.f));
        bool barricadeBroken = world.damageTileAt(sf::Vector2f(4.5f, 1.5f)) == DamageResult::DESTROYED;
        runWorld(world, 0.75f, sf::Vector2f(1.f, 0.f));
        bool standingInSite = world.getPlayer().getCell() == GridCell{7, 1};

        runWorld(world, Round::PlantSeconds + 0.1f, NoInput);
        bool attackersWon = world.getRound().getPhase() == RoundPhase::ATTACKERS_WIN
                            && world.getRound().getEndReason() == RoundEndReason::DEFUSER_PLANTED;
        return actionPhase && barricadeBroken && standingInSite && attackersWon;
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
    runTest("A shot opens a hole that vision passes through", shotOpensHoleThatVisionPassesThrough);
    runTest("End to end: a bullet hole shows the enemy through a wall that still stands",
            endToEndShootingHoleShowsEnemyThroughStandingWall);
    runTest("End to end: a bullet hole gives sight only along the line of fire",
            endToEndHoleGivesSightOnlyAlongLineOfFire);
    runTest("The DDA trace records tMaxX, tMaxY, and the axis of every step", ddaTraceRecordsEveryStep);
    runTest("A* keeps g and the parent of every expanded cell", aStarKeepsCostsAndParentsOfExpandedCells);
    runTest("Tiles sit contiguously: cell (12, 7) is 1392 bytes after cell (0, 0)", tilesSitContiguouslyInMemory);
    runTest("End to end: the DDA trace agrees with line of sight, before and after a bullet hole",
            endToEndDdaTraceAgreesWithLineOfSight);
    runTest("End to end: along the enemy's route, each step adds its own cost to g",
            endToEndRouteCostsAddUpStepByStep);
    runTest("Reinforcing turns drywall into steel that blocks walking and vision", reinforcingTurnsDrywallIntoSteel);
    runTest("Bullets cannot open a reinforced wall", bulletsCannotOpenReinforcedWall);
    runTest("A hard breach charge burns through a reinforced wall after its 3-second fuse",
            hardBreachChargeBurnsThroughAfterFuse);
    runTest("A breach charge opens a soft wall and its soft neighbors, but not steel",
            breachChargeOpensSoftWallAndSoftNeighbors);
    runTest("A barricade blocks the doorway and breaks in one shot", barricadeBlocksDoorwayAndBreaksInOneShot);
    runTest("The drone is faster than the attacker and casts its own 360 rays", droneIsFastAndCastsItsOwnRays);
    runTest("What the drone sees is spotted, and the mark fades after 4 seconds", droneSpotsEnemyAndMarkFades);
    runTest("Holding the bomb site for 4 seconds plants the defuser and wins", holdingBombSitePlantsDefuserAndWins);
    runTest("The defender reinforces its planned walls during preparation",
            defenderReinforcesPlannedWallsDuringPreparation);
    runTest("When the action phase runs out, the defenders win", runningOutOfTimeMeansDefendersWin);
    runTest("When the defender reaches the attacker, the defenders win", defenderReachingAttackerWinsRound);
    runTest("The defender guards the site until it spots the attacker, then hunts", defenderGuardsUntilItSpotsAttacker);
    runTest("End to end: the defender reinforces, the attacker hard-breaches, and the defender hunts through the breach",
            endToEndDefenderReinforcesAttackerBreachesDefenderHunts);
    runTest("End to end: the attacker breaks a barricade, holds the site, and wins",
            endToEndAttackerBreaksBarricadeAndPlants);

    std::cout << '\n' << passedTestCount << " passed, " << failedTestCount << " failed\n";
    if (failedTestCount > 0)
    {
        return 1;
    }
    return 0;
}

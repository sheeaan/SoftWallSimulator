// Framework-free tests for the simulation layer. Each test prints PASS or FAIL.
// The process exits with 1 if any test fails, so a build script can check it.

#include "simulation/Grid.hpp"
#include "simulation/Raycaster.hpp"
#include "simulation/SimulatorErrors.hpp"

#include <cmath>
#include <iostream>
#include <string>

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
}

int main()
{
    std::cout << "Soft Wall Simulator - simulation tests\n\n";

    runTest("Grid stores cells in row-major order in one vector", gridStoresCellsInRowMajorOrder);
    runTest("Vision casts exactly 360 rays", visionCastsExactly360Rays);
    runTest("A ray stops at the first non-EMPTY tile", rayStopsAtFirstNonEmptyTile);

    std::cout << '\n' << passedTestCount << " passed, " << failedTestCount << " failed\n";
    if (failedTestCount > 0)
    {
        return 1;
    }
    return 0;
}

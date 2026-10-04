// Framework-free tests for the simulation layer. Each test prints PASS or FAIL.
// The process exits with 1 if any test fails, so a build script can check it.

#include "simulation/Grid.hpp"
#include "simulation/SimulatorErrors.hpp"

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
}

int main()
{
    std::cout << "Soft Wall Simulator - simulation tests\n\n";

    runTest("Grid stores cells in row-major order in one vector", gridStoresCellsInRowMajorOrder);

    std::cout << '\n' << passedTestCount << " passed, " << failedTestCount << " failed\n";
    if (failedTestCount > 0)
    {
        return 1;
    }
    return 0;
}

#include "simulation/Level.hpp"

#include "simulation/SimulatorErrors.hpp"

#include <utility>

Level parseLevel(const std::vector<std::string>& rows)
{
    if (rows.empty())
    {
        throw LevelFormatError("it has no rows.");
    }
    if (rows[0].empty())
    {
        throw LevelFormatError("row 0 is empty.");
    }

    int rowCount = static_cast<int>(rows.size());
    int columnCount = static_cast<int>(rows[0].size());
    Grid grid(columnCount, rowCount);

    GridCell playerSpawn;
    GridCell enemySpawn;
    int playerSpawnCount = 0;
    int enemySpawnCount = 0;
    std::vector<GridCell> plannedReinforcements;
    std::vector<GridCell> bombSite;
    std::vector<GridCell> guardPoints;

    for (int row = 0; row < rowCount; ++row)
    {
        const std::string& rowText = rows[static_cast<std::size_t>(row)];
        if (static_cast<int>(rowText.size()) != columnCount)
        {
            throw LevelFormatError("row " + std::to_string(row) + " has " + std::to_string(rowText.size())
                                   + " characters, but row 0 has " + std::to_string(columnCount) + ".");
        }

        for (int column = 0; column < columnCount; ++column)
        {
            GridCell cell{column, row};
            char symbol = rowText[static_cast<std::size_t>(column)];
            switch (symbol)
            {
            case '#':
                grid.setTile(cell, TileType::HARD_WALL);
                break;
            case '+':
                grid.setTile(cell, TileType::SOFT_WALL);
                break;
            case 'r':
                grid.setTile(cell, TileType::SOFT_WALL);
                plannedReinforcements.push_back(cell);
                break;
            case 'R':
                grid.setTile(cell, TileType::REINFORCED_WALL);
                break;
            case '=':
                grid.setTile(cell, TileType::BARRICADE);
                break;
            case '.':
                break;
            case '*':
                bombSite.push_back(cell);
                break;
            case 'G':
                bombSite.push_back(cell);
                guardPoints.push_back(cell);
                break;
            case 'P':
                playerSpawn = cell;
                playerSpawnCount += 1;
                break;
            case 'E':
                enemySpawn = cell;
                enemySpawnCount += 1;
                break;
            default:
                throw LevelFormatError("unknown character '" + std::string(1, symbol) + "' at column "
                                       + std::to_string(column) + ", row " + std::to_string(row) + ".");
            }
        }
    }

    if (playerSpawnCount != 1)
    {
        throw LevelFormatError("it needs exactly one 'P' (player spawn), but has "
                               + std::to_string(playerSpawnCount) + ".");
    }
    if (enemySpawnCount != 1)
    {
        throw LevelFormatError("it needs exactly one 'E' (enemy spawn), but has "
                               + std::to_string(enemySpawnCount) + ".");
    }

    return Level{std::move(grid), playerSpawn, enemySpawn, std::move(plannedReinforcements), std::move(bombSite),
                 std::move(guardPoints)};
}

const std::vector<std::string>& getDefaultLevelRows()
{
    // Three wings: west (the attacker's spawn), a central hall, and east (the defender's).
    // The bomb site sits in the middle of the east wing. During the preparation phase the defender
    // reinforces the walls marked 'r' around it. Barricades close the east doorways, so the defender's
    // only way out is the gap at the bottom of the site room, into the south-east room.
    static const std::vector<std::string> rows = {
        "################################################",
        "#...............................#..............#",
        "#.##.......++...................#..++.......##.#",
        "#.##.......++..#................+..++.......##.#",
        "#..............#...##......##...+..............#",
        "#....+++.......#...##......##...+......+++.....#",
        "#..............#................#..............#",
        "#..............#.......++.......#..............#",
        "###++#######..##.......++.......##==######rr####",
        "#..............#................#..............#",
        "#...#.....#....#...##......##...#....#.....#...#",
        "#..............#...##......##...#.....**G**....#",
        "#..............+................r.....*****....#",
        "#....P.........+................r.....*****E...#",
        "#..............+................r.....*****....#",
        "#..............#...##......##...#.....**G**....#",
        "#...#.....#....#...##......##...#....#.....#...#",
        "#..............#................#..............#",
        "##..######++####.......++.......####rr######..##",
        "#..............#.......++.......#..............#",
        "#..+++....##...#................#...##....+++..#",
        "#..+......##...+...##......##...#...##......+..#",
        "#..............+...##......##...#..............#",
        "#......##......#................#......##......#",
        "#......##......#................=......##......#",
        "#..............#................=..............#",
        "################################################",
    };
    return rows;
}

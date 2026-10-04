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
            case '.':
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

    return Level{std::move(grid), playerSpawn, enemySpawn};
}

const std::vector<std::string>& getDefaultLevelRows()
{
    // Three wings: west (player), a central hall, and east (enemy).
    // The open route between P and E winds through the doors in the corners.
    // The soft walls in the dividers are shortcuts the player can break open.
    static const std::vector<std::string> rows = {
        "################################################",
        "#...............................#..............#",
        "#.##.......++...................#..++.......##.#",
        "#.##.......++..#................+..++.......##.#",
        "#..............#...##......##...+..............#",
        "#....+++.......#...##......##...+......+++.....#",
        "#..............#................#..............#",
        "#..............#.......++.......#..............#",
        "###++#######..##.......++.......##..######++####",
        "#..............#................#..............#",
        "#...#.....#....#...##......##...#....#.....#...#",
        "#..............#...##......##...#..............#",
        "#..............+................+..............#",
        "#....P.........+................+..........E...#",
        "#..............+................+..............#",
        "#..............#...##......##...#..............#",
        "#...#.....#....#...##......##...#....#.....#...#",
        "#..............#................#..............#",
        "##..######++####.......++.......####++######..##",
        "#..............#.......++.......#..............#",
        "#..+++....##...#................#...##....+++..#",
        "#..+......##...+...##......##...#...##......+..#",
        "#..............+...##......##...#..............#",
        "#......##......#................#......##......#",
        "#......##......#.......................##......#",
        "#..............#...............................#",
        "################################################",
    };
    return rows;
}

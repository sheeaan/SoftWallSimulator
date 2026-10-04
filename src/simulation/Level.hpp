#pragma once

#include "simulation/Grid.hpp"

#include <string>
#include <vector>

struct Level
{
    Grid grid;
    GridCell playerSpawn;
    GridCell enemySpawn;
};

// Builds a level from ASCII rows: '#' HARD_WALL, '+' SOFT_WALL, '.' EMPTY,
// 'P' player spawn, 'E' enemy spawn. Spawn cells are EMPTY.
// Throws LevelFormatError when the rows differ in width, a character is unknown,
// or there is not exactly one 'P' and one 'E'.
Level parseLevel(const std::vector<std::string>& rows);

// The hand-made 48 x 27 map the game starts with.
const std::vector<std::string>& getDefaultLevelRows();

#pragma once

#include "simulation/Grid.hpp"

#include <string>
#include <vector>

struct Level
{
    Grid grid;
    GridCell playerSpawn;
    GridCell enemySpawn;
    std::vector<GridCell> plannedReinforcements; // Soft walls the defender reinforces during the preparation phase.
    std::vector<GridCell> bombSite;              // Floor cells of the objective. Empty means no round: free play.
    std::vector<GridCell> guardPoints;           // Where the defender stands watch, inside the bomb site.
};

// Builds a level from ASCII rows:
//   '#' HARD_WALL       '+' SOFT_WALL        'r' SOFT_WALL the defender plans to reinforce
//   'R' REINFORCED_WALL '=' BARRICADE        '.' EMPTY
//   '*' bomb site floor 'G' bomb site floor where the defender stands guard
//   'P' player spawn    'E' enemy spawn (both EMPTY)
// Throws LevelFormatError when the rows differ in width, a character is unknown,
// or there is not exactly one 'P' and one 'E'.
Level parseLevel(const std::vector<std::string>& rows);

// The hand-made 48 x 27 map the game starts with.
const std::vector<std::string>& getDefaultLevelRows();

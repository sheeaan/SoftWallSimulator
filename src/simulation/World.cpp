#include "simulation/World.hpp"

World::World(const Level& level)
    : grid(level.grid)
    , player(cellCenter(level.playerSpawn))
{
}

void World::update(float seconds, sf::Vector2f playerInputDirection)
{
    player.move(playerInputDirection, seconds, grid);
}

const Grid& World::getGrid() const
{
    return grid;
}

const Player& World::getPlayer() const
{
    return player;
}

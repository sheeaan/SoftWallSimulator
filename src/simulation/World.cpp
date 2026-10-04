#include "simulation/World.hpp"

World::World(const Level& level)
    : grid(level.grid)
    , player(cellCenter(level.playerSpawn))
{
    raycaster.castVisibility(player.getPosition(), grid);
}

void World::update(float seconds, sf::Vector2f playerInputDirection)
{
    player.move(playerInputDirection, seconds, grid);
    raycaster.castVisibility(player.getPosition(), grid);
}

const Grid& World::getGrid() const
{
    return grid;
}

const Player& World::getPlayer() const
{
    return player;
}

const std::vector<RayHit>& World::getVisibility() const
{
    return raycaster.getVisibility();
}

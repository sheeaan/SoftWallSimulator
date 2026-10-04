#pragma once

#include "simulation/Grid.hpp"
#include "simulation/Level.hpp"
#include "simulation/Player.hpp"

#include <SFML/System/Vector2.hpp>

// The whole simulation. The presentation layer drives it through update() and reads it through const getters.
class World
{
public:
    explicit World(const Level& level);

    void update(float seconds, sf::Vector2f playerInputDirection);

    const Grid& getGrid() const;
    const Player& getPlayer() const;

private:
    Grid grid;
    Player player;
};

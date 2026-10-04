#pragma once

#include "simulation/Grid.hpp"

#include <SFML/System/Vector2.hpp>

class Player
{
public:
    static constexpr float Speed = 6.0f;    // Tiles per second.
    static constexpr float HalfSize = 0.3f; // Half the width of the collision box, in tiles.

    explicit Player(sf::Vector2f spawnPosition);

    // Moves along the input direction and slides along walls instead of entering them.
    void move(sf::Vector2f inputDirection, float seconds, const Grid& grid);

    sf::Vector2f getPosition() const;
    GridCell getCell() const;

private:
    bool overlapsBlockedTile(sf::Vector2f candidatePosition, const Grid& grid) const;

    sf::Vector2f position;
};

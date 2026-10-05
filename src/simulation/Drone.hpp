#pragma once

#include "simulation/Grid.hpp"

#include <SFML/System/Vector2.hpp>

// The attacker's scout. It drives like the player, only faster and smaller, and it rolls under barricades.
class Drone
{
public:
    static constexpr float Speed = 9.0f;     // Tiles per second, half again as fast as the player.
    static constexpr float HalfSize = 0.15f; // Half the width of the collision box, in tiles.

    explicit Drone(sf::Vector2f startPosition);

    // Moves along the input direction and slides along anything a drone cannot pass.
    void move(sf::Vector2f inputDirection, float seconds, const Grid& grid);

    sf::Vector2f getPosition() const;
    GridCell getCell() const;

private:
    bool overlapsBlockedTile(sf::Vector2f candidatePosition, const Grid& grid) const;

    sf::Vector2f position;
};

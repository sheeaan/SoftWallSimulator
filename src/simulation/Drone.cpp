#include "simulation/Drone.hpp"

Drone::Drone(sf::Vector2f startPosition)
    : position(startPosition)
{
}

void Drone::move(sf::Vector2f inputDirection, float seconds, const Grid& grid)
{
    if (inputDirection == sf::Vector2f(0.f, 0.f))
    {
        return;
    }

    // The same axis-by-axis slide as the player, with the drone's speed and size.
    sf::Vector2f displacement = inputDirection.normalized() * (Speed * seconds);

    sf::Vector2f candidatePosition = position;
    candidatePosition.x += displacement.x;
    if (!overlapsBlockedTile(candidatePosition, grid))
    {
        position = candidatePosition;
    }

    candidatePosition = position;
    candidatePosition.y += displacement.y;
    if (!overlapsBlockedTile(candidatePosition, grid))
    {
        position = candidatePosition;
    }
}

sf::Vector2f Drone::getPosition() const
{
    return position;
}

GridCell Drone::getCell() const
{
    return cellContaining(position);
}

bool Drone::overlapsBlockedTile(sf::Vector2f candidatePosition, const Grid& grid) const
{
    GridCell topLeft = cellContaining(sf::Vector2f(candidatePosition.x - HalfSize, candidatePosition.y - HalfSize));
    GridCell bottomRight = cellContaining(sf::Vector2f(candidatePosition.x + HalfSize, candidatePosition.y + HalfSize));

    for (int row = topLeft.row; row <= bottomRight.row; ++row)
    {
        for (int column = topLeft.column; column <= bottomRight.column; ++column)
        {
            if (!grid.isDronePassable(GridCell{column, row}))
            {
                return true;
            }
        }
    }
    return false;
}

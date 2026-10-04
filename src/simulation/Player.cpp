#include "simulation/Player.hpp"

Player::Player(sf::Vector2f spawnPosition)
    : position(spawnPosition)
{
}

void Player::move(sf::Vector2f inputDirection, float seconds, const Grid& grid)
{
    if (inputDirection == sf::Vector2f(0.f, 0.f))
    {
        return;
    }

    // Normalize so that moving diagonally is not faster than moving straight.
    sf::Vector2f displacement = inputDirection.normalized() * (Speed * seconds);

    // Move one axis at a time. A blocked axis is skipped on its own, so the player slides along walls.
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

sf::Vector2f Player::getPosition() const
{
    return position;
}

GridCell Player::getCell() const
{
    return cellContaining(position);
}

bool Player::overlapsBlockedTile(sf::Vector2f candidatePosition, const Grid& grid) const
{
    // The box is smaller than a tile, so it touches at most a 2 x 2 block of cells.
    GridCell topLeft = cellContaining(sf::Vector2f(candidatePosition.x - HalfSize, candidatePosition.y - HalfSize));
    GridCell bottomRight = cellContaining(sf::Vector2f(candidatePosition.x + HalfSize, candidatePosition.y + HalfSize));

    for (int row = topLeft.row; row <= bottomRight.row; ++row)
    {
        for (int column = topLeft.column; column <= bottomRight.column; ++column)
        {
            if (!grid.isWalkable(GridCell{column, row}))
            {
                return true;
            }
        }
    }
    return false;
}

#pragma once

#include "simulation/Grid.hpp"

#include <SFML/System/Vector2.hpp>

#include <cstddef>
#include <vector>

// Walks a path of cells at a constant speed. World decides when the path changes.
class Enemy
{
public:
    static constexpr float Speed = 4.0f;  // Tiles per second. Slower than the player, so the player can escape.
    static constexpr float Radius = 0.3f; // Tiles.

    explicit Enemy(sf::Vector2f spawnPosition);

    // Replaces the path. The path starts in the cell the enemy stands in, so the enemy
    // heads straight for the second cell instead of first walking back to its own cell's center.
    void setPath(std::vector<GridCell> newPath);
    void followPath(float seconds);

    sf::Vector2f getPosition() const;
    GridCell getCell() const;
    const std::vector<GridCell>& getPath() const;
    std::size_t getNextWaypointIndex() const;

private:
    sf::Vector2f position;
    std::vector<GridCell> path;
    std::size_t nextWaypointIndex = 0;
};

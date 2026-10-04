#pragma once

#include "simulation/Grid.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>

// Draws the grid from two vertex arrays, so the floor and the walls are one draw call each.
class TileMapRenderer
{
public:
    explicit TileMapRenderer(const Grid& grid);

    // Call after any tile changes.
    void rebuildWalls(const Grid& grid);

    void drawFloor(sf::RenderTarget& target) const;
    void drawWalls(sf::RenderTarget& target) const;

private:
    void appendHardWall(const Grid& grid, GridCell cell);
    void appendSoftWall(GridCell cell);

    sf::VertexArray floorVertices;
    sf::VertexArray wallVertices;
};

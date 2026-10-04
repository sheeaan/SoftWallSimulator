#pragma once

#include "simulation/Grid.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>

// Draws the grid from vertex arrays, so each layer is one draw call.
// Floor and wall bodies are drawn before the light map, so darkness covers them.
// Wall edges are drawn after it, so the outline of the map stays readable in the dark.
class TileMapRenderer
{
public:
    explicit TileMapRenderer(const Grid& grid);

    // Call after any tile changes.
    void rebuildWalls(const Grid& grid);

    void drawFloor(sf::RenderTarget& target) const;
    void drawWalls(sf::RenderTarget& target) const;
    void drawWallEdges(sf::RenderTarget& target) const;

private:
    void appendHardWall(const Grid& grid, GridCell cell);
    void appendSoftWall(GridCell cell);

    sf::VertexArray floorVertices;
    sf::VertexArray wallVertices;
    sf::VertexArray wallEdgeVertices;
};

#pragma once

#include "simulation/Grid.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <vector>

// Draws the grid from vertex arrays, so each layer is one draw call.
// Soft walls are drywall over wooden studs; hard walls are reinforced steel plates.
// Floor and wall bodies are drawn before the light map, so darkness covers them.
// Wall edges are drawn after it, so the outline of the map stays readable in the dark.
class TileMapRenderer
{
public:
    explicit TileMapRenderer(const Grid& grid);

    // Call after any tile changes.
    void rebuildWalls(const Grid& grid);

    // Flashes a tile white for a moment, to show it was hit.
    void flashTile(GridCell cell);
    void update(float seconds);

    void drawFloor(sf::RenderTarget& target) const;
    void drawWalls(sf::RenderTarget& target) const;
    void drawWallEdges(sf::RenderTarget& target) const;
    void drawFlashes(sf::RenderTarget& target);

private:
    struct TileFlash
    {
        GridCell cell;
        float secondsLeft = 0.f;
    };

    void appendSteelWall(const Grid& grid, GridCell cell);
    void appendDrywall(const Grid& grid, GridCell cell, const Tile& tile);

    sf::VertexArray floorVertices;
    sf::VertexArray wallVertices;
    sf::VertexArray wallEdgeVertices;
    sf::VertexArray flashVertices;
    std::vector<TileFlash> flashes;
};

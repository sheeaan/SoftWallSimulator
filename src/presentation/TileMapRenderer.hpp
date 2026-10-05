#pragma once

#include "simulation/Grid.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <vector>

// Draws the grid from vertex arrays, so each layer is one draw call.
// Soft walls are drywall over wooden studs, hard walls are concrete, reinforced walls are braced steel plates,
// and barricades are wooden planks across a doorway. The bomb site is marked on the floor.
// Floor and wall bodies are drawn before the light map, so darkness covers them.
// Wall edges and the site outline are drawn after it, so the map stays readable in the dark.
class TileMapRenderer
{
public:
    TileMapRenderer(const Grid& grid, const std::vector<GridCell>& bombSite);

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

    void appendConcreteWall(const Grid& grid, GridCell cell);
    void appendReinforcedWall(const Grid& grid, GridCell cell);
    void appendDrywall(const Grid& grid, GridCell cell, const Tile& tile);
    void appendBarricade(const Grid& grid, GridCell cell);
    void appendFloorFacingEdges(const Grid& grid, GridCell cell, sf::Color color);

    sf::VertexArray floorVertices;
    sf::VertexArray siteOutlineVertices;
    sf::VertexArray wallVertices;
    sf::VertexArray wallEdgeVertices;
    sf::VertexArray flashVertices;
    std::vector<TileFlash> flashes;
};

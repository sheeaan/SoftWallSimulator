#include "presentation/TileMapRenderer.hpp"

#include "presentation/Palette.hpp"
#include "presentation/VertexQuads.hpp"

namespace
{
    constexpr float FloorSeamWidth = 0.03f;  // Gap between floor tiles that shows as faint grid lines.
    constexpr float HardWallEdgeWidth = 0.1f; // Bright rim on hard-wall sides that face open floor.
    constexpr float SoftWallInset = 0.06f;   // Gap around each soft wall, so each one reads as a separate block.
    constexpr float SoftWallEdgeWidth = 0.1f;
}

TileMapRenderer::TileMapRenderer(const Grid& grid)
    : floorVertices(sf::PrimitiveType::Triangles)
    , wallVertices(sf::PrimitiveType::Triangles)
{
    // The floor never changes, so it is built once. Walls draw on top of it.
    // Rows outside, columns inside: the same order the tiles sit in memory.
    for (int row = 0; row < grid.getRowCount(); ++row)
    {
        for (int column = 0; column < grid.getColumnCount(); ++column)
        {
            // A faint checkerboard gives the floor some texture without image files.
            sf::Color floorColor = Palette::FloorDark;
            if ((column + row) % 2 == 0)
            {
                floorColor = Palette::FloorLight;
            }

            float left = static_cast<float>(column);
            float top = static_cast<float>(row);
            appendQuad(floorVertices,
                       sf::Vector2f(left + FloorSeamWidth, top + FloorSeamWidth),
                       sf::Vector2f(left + 1.f - FloorSeamWidth, top + 1.f - FloorSeamWidth),
                       floorColor);
        }
    }

    rebuildWalls(grid);
}

void TileMapRenderer::rebuildWalls(const Grid& grid)
{
    wallVertices.clear();
    for (int row = 0; row < grid.getRowCount(); ++row)
    {
        for (int column = 0; column < grid.getColumnCount(); ++column)
        {
            GridCell cell{column, row};
            TileType type = grid.getTile(cell).type;
            if (type == TileType::HARD_WALL)
            {
                appendHardWall(grid, cell);
            }
            else if (type == TileType::SOFT_WALL)
            {
                appendSoftWall(cell);
            }
        }
    }
}

void TileMapRenderer::drawFloor(sf::RenderTarget& target) const
{
    target.draw(floorVertices);
}

void TileMapRenderer::drawWalls(sf::RenderTarget& target) const
{
    target.draw(wallVertices);
}

void TileMapRenderer::appendHardWall(const Grid& grid, GridCell cell)
{
    float left = static_cast<float>(cell.column);
    float top = static_cast<float>(cell.row);
    float right = left + 1.f;
    float bottom = top + 1.f;

    appendQuad(wallVertices, sf::Vector2f(left, top), sf::Vector2f(right, bottom), Palette::HardWall);

    // A rim on each side that faces walkable floor outlines the walls like a blueprint.
    if (grid.isWalkable(GridCell{cell.column, cell.row - 1}))
    {
        appendQuad(wallVertices, sf::Vector2f(left, top), sf::Vector2f(right, top + HardWallEdgeWidth),
                   Palette::HardWallEdge);
    }
    if (grid.isWalkable(GridCell{cell.column, cell.row + 1}))
    {
        appendQuad(wallVertices, sf::Vector2f(left, bottom - HardWallEdgeWidth), sf::Vector2f(right, bottom),
                   Palette::HardWallEdge);
    }
    if (grid.isWalkable(GridCell{cell.column - 1, cell.row}))
    {
        appendQuad(wallVertices, sf::Vector2f(left, top), sf::Vector2f(left + HardWallEdgeWidth, bottom),
                   Palette::HardWallEdge);
    }
    if (grid.isWalkable(GridCell{cell.column + 1, cell.row}))
    {
        appendQuad(wallVertices, sf::Vector2f(right - HardWallEdgeWidth, top), sf::Vector2f(right, bottom),
                   Palette::HardWallEdge);
    }
}

void TileMapRenderer::appendSoftWall(GridCell cell)
{
    float left = static_cast<float>(cell.column) + SoftWallInset;
    float top = static_cast<float>(cell.row) + SoftWallInset;
    float right = static_cast<float>(cell.column) + 1.f - SoftWallInset;
    float bottom = static_cast<float>(cell.row) + 1.f - SoftWallInset;

    // A light border with a darker body, like a crate seen from above.
    appendQuad(wallVertices, sf::Vector2f(left, top), sf::Vector2f(right, bottom), Palette::SoftWallEdge);
    appendQuad(wallVertices,
               sf::Vector2f(left + SoftWallEdgeWidth, top + SoftWallEdgeWidth),
               sf::Vector2f(right - SoftWallEdgeWidth, bottom - SoftWallEdgeWidth),
               Palette::SoftWall);
}

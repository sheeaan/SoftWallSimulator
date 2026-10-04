#include "presentation/TileMapRenderer.hpp"

#include "presentation/Palette.hpp"
#include "presentation/VertexQuads.hpp"

#include <SFML/Graphics/RenderStates.hpp>

namespace
{
    constexpr float FloorSeamWidth = 0.03f;  // Gap between floor tiles that shows as faint grid lines.
    constexpr float HardWallEdgeWidth = 0.1f; // Bright rim on hard-wall sides that face open floor.
    constexpr float SoftWallInset = 0.06f;   // Gap around each soft wall, so each one reads as a separate block.
    constexpr float SoftWallEdgeWidth = 0.1f;
    constexpr float FlashSeconds = 0.12f;

    struct CrackSegment
    {
        sf::Vector2f from; // Tile-local coordinates, 0 to 1.
        sf::Vector2f to;
    };

    // The first crack appears after one hit. The second joins it after two.
    constexpr CrackSegment FirstCrack[] = {
        {sf::Vector2f(0.22f, 0.12f), sf::Vector2f(0.40f, 0.38f)},
        {sf::Vector2f(0.40f, 0.38f), sf::Vector2f(0.34f, 0.58f)},
        {sf::Vector2f(0.34f, 0.58f), sf::Vector2f(0.52f, 0.82f)},
    };
    constexpr CrackSegment SecondCrack[] = {
        {sf::Vector2f(0.86f, 0.20f), sf::Vector2f(0.63f, 0.42f)},
        {sf::Vector2f(0.63f, 0.42f), sf::Vector2f(0.70f, 0.62f)},
        {sf::Vector2f(0.70f, 0.62f), sf::Vector2f(0.56f, 0.88f)},
        {sf::Vector2f(0.40f, 0.38f), sf::Vector2f(0.63f, 0.42f)},
    };

    sf::Color darken(sf::Color color, float brightness)
    {
        return sf::Color(static_cast<std::uint8_t>(color.r * brightness),
                         static_cast<std::uint8_t>(color.g * brightness),
                         static_cast<std::uint8_t>(color.b * brightness));
    }
}

TileMapRenderer::TileMapRenderer(const Grid& grid)
    : floorVertices(sf::PrimitiveType::Triangles)
    , wallVertices(sf::PrimitiveType::Triangles)
    , crackVertices(sf::PrimitiveType::Lines)
    , wallEdgeVertices(sf::PrimitiveType::Triangles)
    , flashVertices(sf::PrimitiveType::Triangles)
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
    crackVertices.clear();
    wallEdgeVertices.clear();
    for (int row = 0; row < grid.getRowCount(); ++row)
    {
        for (int column = 0; column < grid.getColumnCount(); ++column)
        {
            GridCell cell{column, row};
            const Tile& tile = grid.getTile(cell);
            if (tile.type == TileType::HARD_WALL)
            {
                appendHardWall(grid, cell);
            }
            else if (tile.type == TileType::SOFT_WALL)
            {
                appendSoftWall(cell, tile.health);
            }
        }
    }
}

void TileMapRenderer::flashTile(GridCell cell)
{
    TileFlash flash;
    flash.cell = cell;
    flash.secondsLeft = FlashSeconds;
    flashes.push_back(flash);
}

void TileMapRenderer::update(float seconds)
{
    std::size_t index = 0;
    while (index < flashes.size())
    {
        flashes[index].secondsLeft -= seconds;
        if (flashes[index].secondsLeft <= 0.f)
        {
            // Swap and pop. The moved flash has not been updated yet, so the index stays.
            flashes[index] = flashes.back();
            flashes.pop_back();
            continue;
        }
        index += 1;
    }
}

void TileMapRenderer::drawFloor(sf::RenderTarget& target) const
{
    target.draw(floorVertices);
}

void TileMapRenderer::drawWalls(sf::RenderTarget& target) const
{
    target.draw(wallVertices);
    target.draw(crackVertices);
}

void TileMapRenderer::drawWallEdges(sf::RenderTarget& target) const
{
    target.draw(wallEdgeVertices);
}

void TileMapRenderer::drawFlashes(sf::RenderTarget& target)
{
    flashVertices.clear();
    for (const TileFlash& flash : flashes)
    {
        sf::Color color = Palette::TileFlash;
        color.a = static_cast<std::uint8_t>(color.a * (flash.secondsLeft / FlashSeconds));
        sf::Vector2f topLeft(static_cast<float>(flash.cell.column), static_cast<float>(flash.cell.row));
        appendQuad(flashVertices, topLeft, topLeft + sf::Vector2f(1.f, 1.f), color);
    }
    target.draw(flashVertices, sf::BlendAdd);
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
        appendQuad(wallEdgeVertices, sf::Vector2f(left, top), sf::Vector2f(right, top + HardWallEdgeWidth),
                   Palette::HardWallEdge);
    }
    if (grid.isWalkable(GridCell{cell.column, cell.row + 1}))
    {
        appendQuad(wallEdgeVertices, sf::Vector2f(left, bottom - HardWallEdgeWidth), sf::Vector2f(right, bottom),
                   Palette::HardWallEdge);
    }
    if (grid.isWalkable(GridCell{cell.column - 1, cell.row}))
    {
        appendQuad(wallEdgeVertices, sf::Vector2f(left, top), sf::Vector2f(left + HardWallEdgeWidth, bottom),
                   Palette::HardWallEdge);
    }
    if (grid.isWalkable(GridCell{cell.column + 1, cell.row}))
    {
        appendQuad(wallEdgeVertices, sf::Vector2f(right - HardWallEdgeWidth, top), sf::Vector2f(right, bottom),
                   Palette::HardWallEdge);
    }
}

void TileMapRenderer::appendSoftWall(GridCell cell, std::uint8_t health)
{
    float left = static_cast<float>(cell.column) + SoftWallInset;
    float top = static_cast<float>(cell.row) + SoftWallInset;
    float right = static_cast<float>(cell.column) + 1.f - SoftWallInset;
    float bottom = static_cast<float>(cell.row) + 1.f - SoftWallInset;

    // The body darkens with each hit: full color at 3 health, 85% at 2, 70% at 1.
    float brightness = 0.55f + 0.15f * static_cast<float>(health);
    appendQuad(wallVertices, sf::Vector2f(left, top), sf::Vector2f(right, bottom), darken(Palette::SoftWall, brightness));

    // A bright border, like a crate seen from above.
    appendQuad(wallEdgeVertices, sf::Vector2f(left, top), sf::Vector2f(right, top + SoftWallEdgeWidth),
               Palette::SoftWallEdge);
    appendQuad(wallEdgeVertices, sf::Vector2f(left, bottom - SoftWallEdgeWidth), sf::Vector2f(right, bottom),
               Palette::SoftWallEdge);
    appendQuad(wallEdgeVertices, sf::Vector2f(left, top), sf::Vector2f(left + SoftWallEdgeWidth, bottom),
               Palette::SoftWallEdge);
    appendQuad(wallEdgeVertices, sf::Vector2f(right - SoftWallEdgeWidth, top), sf::Vector2f(right, bottom),
               Palette::SoftWallEdge);

    // Cracks spread as health drops.
    sf::Vector2f tileOrigin(static_cast<float>(cell.column), static_cast<float>(cell.row));
    if (health < Grid::SoftWallMaxHealth)
    {
        for (const CrackSegment& segment : FirstCrack)
        {
            crackVertices.append(sf::Vertex{tileOrigin + segment.from, Palette::Crack});
            crackVertices.append(sf::Vertex{tileOrigin + segment.to, Palette::Crack});
        }
    }
    if (health < Grid::SoftWallMaxHealth - 1)
    {
        for (const CrackSegment& segment : SecondCrack)
        {
            crackVertices.append(sf::Vertex{tileOrigin + segment.from, Palette::Crack});
            crackVertices.append(sf::Vertex{tileOrigin + segment.to, Palette::Crack});
        }
    }
}

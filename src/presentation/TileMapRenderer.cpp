#include "presentation/TileMapRenderer.hpp"

#include "presentation/Palette.hpp"
#include "presentation/VertexQuads.hpp"

#include <SFML/Graphics/RenderStates.hpp>

#include <algorithm>
#include <cstdint>

namespace
{
    constexpr float FloorSeamWidth = 0.03f;    // Gap between floor tiles that shows as faint grid lines.
    constexpr float EdgeWidth = 0.1f;          // Outline on wall sides that face open floor.
    constexpr float DrywallSeamWidth = 0.015f; // Hairline joint between neighboring drywall boards.
    constexpr float SplinterWidth = 0.05f;     // Torn board and exposed stud around a bullet hole.
    constexpr float SteelFrameWidth = 0.1f;
    constexpr float RivetSize = 0.07f;
    constexpr float RivetInset = 0.17f;
    constexpr float FlashSeconds = 0.12f;
    constexpr float PanelSize = 1.f / static_cast<float>(Grid::PanelsPerSide);
}

TileMapRenderer::TileMapRenderer(const Grid& grid)
    : floorVertices(sf::PrimitiveType::Triangles)
    , wallVertices(sf::PrimitiveType::Triangles)
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
    wallEdgeVertices.clear();
    for (int row = 0; row < grid.getRowCount(); ++row)
    {
        for (int column = 0; column < grid.getColumnCount(); ++column)
        {
            GridCell cell{column, row};
            const Tile& tile = grid.getTile(cell);
            if (tile.type == TileType::HARD_WALL)
            {
                appendSteelWall(grid, cell);
            }
            else if (tile.type == TileType::SOFT_WALL)
            {
                appendDrywall(grid, cell, tile);
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

void TileMapRenderer::appendSteelWall(const Grid& grid, GridCell cell)
{
    float left = static_cast<float>(cell.column);
    float top = static_cast<float>(cell.row);
    float right = left + 1.f;
    float bottom = top + 1.f;

    // A reinforced steel plate: a dark frame, a raised inner plate, and a rivet in each corner.
    appendQuad(wallVertices, sf::Vector2f(left, top), sf::Vector2f(right, bottom), Palette::SteelPlate);
    appendQuad(wallVertices,
               sf::Vector2f(left + SteelFrameWidth, top + SteelFrameWidth),
               sf::Vector2f(right - SteelFrameWidth, bottom - SteelFrameWidth),
               Palette::SteelInnerPlate);

    const sf::Vector2f rivetCenters[4] = {
        sf::Vector2f(left + RivetInset, top + RivetInset),
        sf::Vector2f(right - RivetInset, top + RivetInset),
        sf::Vector2f(left + RivetInset, bottom - RivetInset),
        sf::Vector2f(right - RivetInset, bottom - RivetInset),
    };
    sf::Vector2f halfRivet(RivetSize / 2.f, RivetSize / 2.f);
    for (const sf::Vector2f& rivetCenter : rivetCenters)
    {
        appendQuad(wallVertices, rivetCenter - halfRivet, rivetCenter + halfRivet, Palette::Rivet);
    }

    // A rim on each side that faces walkable floor outlines the walls.
    if (grid.isWalkable(GridCell{cell.column, cell.row - 1}))
    {
        appendQuad(wallEdgeVertices, sf::Vector2f(left, top), sf::Vector2f(right, top + EdgeWidth), Palette::SteelEdge);
    }
    if (grid.isWalkable(GridCell{cell.column, cell.row + 1}))
    {
        appendQuad(wallEdgeVertices, sf::Vector2f(left, bottom - EdgeWidth), sf::Vector2f(right, bottom), Palette::SteelEdge);
    }
    if (grid.isWalkable(GridCell{cell.column - 1, cell.row}))
    {
        appendQuad(wallEdgeVertices, sf::Vector2f(left, top), sf::Vector2f(left + EdgeWidth, bottom), Palette::SteelEdge);
    }
    if (grid.isWalkable(GridCell{cell.column + 1, cell.row}))
    {
        appendQuad(wallEdgeVertices, sf::Vector2f(right - EdgeWidth, top), sf::Vector2f(right, bottom), Palette::SteelEdge);
    }
}

void TileMapRenderer::appendDrywall(const Grid& grid, GridCell cell, const Tile& tile)
{
    float left = static_cast<float>(cell.column);
    float top = static_cast<float>(cell.row);

    // The board is inset by a hairline, so neighboring boards show a joint between them.
    float boardLeft = left + DrywallSeamWidth;
    float boardTop = top + DrywallSeamWidth;
    float boardRight = left + 1.f - DrywallSeamWidth;
    float boardBottom = top + 1.f - DrywallSeamWidth;

    // Intact panels are plaster. Punched-out panels are left empty, so the floor shows through the hole.
    for (int panelRow = 0; panelRow < Grid::PanelsPerSide; ++panelRow)
    {
        for (int panelColumn = 0; panelColumn < Grid::PanelsPerSide; ++panelColumn)
        {
            if (!isPanelIntact(tile, panelColumn, panelRow))
            {
                continue;
            }
            float panelLeft = std::max(left + static_cast<float>(panelColumn) * PanelSize, boardLeft);
            float panelTop = std::max(top + static_cast<float>(panelRow) * PanelSize, boardTop);
            float panelRight = std::min(left + static_cast<float>(panelColumn + 1) * PanelSize, boardRight);
            float panelBottom = std::min(top + static_cast<float>(panelRow + 1) * PanelSize, boardBottom);
            appendQuad(wallVertices, sf::Vector2f(panelLeft, panelTop), sf::Vector2f(panelRight, panelBottom),
                       Palette::Plaster);
        }
    }

    // Around each hole, the torn board and the wooden stud behind it show along every intact neighbor.
    for (int panelRow = 0; panelRow < Grid::PanelsPerSide; ++panelRow)
    {
        for (int panelColumn = 0; panelColumn < Grid::PanelsPerSide; ++panelColumn)
        {
            if (isPanelIntact(tile, panelColumn, panelRow))
            {
                continue;
            }
            float holeLeft = left + static_cast<float>(panelColumn) * PanelSize;
            float holeTop = top + static_cast<float>(panelRow) * PanelSize;
            float holeRight = holeLeft + PanelSize;
            float holeBottom = holeTop + PanelSize;

            if (panelRow > 0 && isPanelIntact(tile, panelColumn, panelRow - 1))
            {
                appendQuad(wallVertices, sf::Vector2f(holeLeft, holeTop - SplinterWidth), sf::Vector2f(holeRight, holeTop),
                           Palette::Splinter);
            }
            if (panelRow < Grid::PanelsPerSide - 1 && isPanelIntact(tile, panelColumn, panelRow + 1))
            {
                appendQuad(wallVertices, sf::Vector2f(holeLeft, holeBottom), sf::Vector2f(holeRight, holeBottom + SplinterWidth),
                           Palette::Splinter);
            }
            if (panelColumn > 0 && isPanelIntact(tile, panelColumn - 1, panelRow))
            {
                appendQuad(wallVertices, sf::Vector2f(holeLeft - SplinterWidth, holeTop), sf::Vector2f(holeLeft, holeBottom),
                           Palette::Splinter);
            }
            if (panelColumn < Grid::PanelsPerSide - 1 && isPanelIntact(tile, panelColumn + 1, panelRow))
            {
                appendQuad(wallVertices, sf::Vector2f(holeRight, holeTop), sf::Vector2f(holeRight + SplinterWidth, holeBottom),
                           Palette::Splinter);
            }
        }
    }

    // Outline the sides that face open floor one panel at a time, so a slot that exits the wall leaves a gap.
    bool floorAbove = grid.isWalkable(GridCell{cell.column, cell.row - 1});
    bool floorBelow = grid.isWalkable(GridCell{cell.column, cell.row + 1});
    bool floorLeft = grid.isWalkable(GridCell{cell.column - 1, cell.row});
    bool floorRight = grid.isWalkable(GridCell{cell.column + 1, cell.row});
    int lastPanel = Grid::PanelsPerSide - 1;
    for (int panel = 0; panel < Grid::PanelsPerSide; ++panel)
    {
        float spanStart = static_cast<float>(panel) * PanelSize;
        float spanEnd = spanStart + PanelSize;
        if (floorAbove && isPanelIntact(tile, panel, 0))
        {
            appendQuad(wallEdgeVertices, sf::Vector2f(left + spanStart, top), sf::Vector2f(left + spanEnd, top + EdgeWidth),
                       Palette::DrywallEdge);
        }
        if (floorBelow && isPanelIntact(tile, panel, lastPanel))
        {
            appendQuad(wallEdgeVertices, sf::Vector2f(left + spanStart, top + 1.f - EdgeWidth),
                       sf::Vector2f(left + spanEnd, top + 1.f), Palette::DrywallEdge);
        }
        if (floorLeft && isPanelIntact(tile, 0, panel))
        {
            appendQuad(wallEdgeVertices, sf::Vector2f(left, top + spanStart), sf::Vector2f(left + EdgeWidth, top + spanEnd),
                       Palette::DrywallEdge);
        }
        if (floorRight && isPanelIntact(tile, lastPanel, panel))
        {
            appendQuad(wallEdgeVertices, sf::Vector2f(left + 1.f - EdgeWidth, top + spanStart),
                       sf::Vector2f(left + 1.f, top + spanEnd), Palette::DrywallEdge);
        }
    }
}

#include "simulation/Grid.hpp"

#include "simulation/SimulatorErrors.hpp"

#include <algorithm>
#include <cmath>
#include <string>

bool operator==(GridCell left, GridCell right)
{
    return left.column == right.column && left.row == right.row;
}

bool operator!=(GridCell left, GridCell right)
{
    return !(left == right);
}

GridCell cellContaining(sf::Vector2f worldPosition)
{
    // std::floor, not a cast: a cast rounds toward zero and would put x = -0.5 inside column 0.
    GridCell cell;
    cell.column = static_cast<int>(std::floor(worldPosition.x));
    cell.row = static_cast<int>(std::floor(worldPosition.y));
    return cell;
}

sf::Vector2f cellCenter(GridCell cell)
{
    return sf::Vector2f(static_cast<float>(cell.column) + 0.5f, static_cast<float>(cell.row) + 0.5f);
}

Grid::Grid(int columns, int rows)
    : columnCount(columns)
    , rowCount(rows)
{
    if (columns <= 0 || rows <= 0)
    {
        throw GridBoundsError("A grid needs at least one column and one row, but was given "
                              + std::to_string(columns) + " x " + std::to_string(rows) + ".");
    }

    // One allocation for the whole map. Every tile starts EMPTY.
    tiles.resize(static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows));
}

int Grid::getColumnCount() const
{
    return columnCount;
}

int Grid::getRowCount() const
{
    return rowCount;
}

std::size_t Grid::getCellCount() const
{
    return tiles.size();
}

bool Grid::contains(GridCell cell) const
{
    bool columnInside = cell.column >= 0 && cell.column < columnCount;
    bool rowInside = cell.row >= 0 && cell.row < rowCount;
    return columnInside && rowInside;
}

std::size_t Grid::indexOf(GridCell cell) const
{
    if (!contains(cell))
    {
        throw GridBoundsError("Cell (column " + std::to_string(cell.column) + ", row " + std::to_string(cell.row)
                              + ") is outside the " + std::to_string(columnCount) + " x "
                              + std::to_string(rowCount) + " grid.");
    }

    // Row-major order: all of row 0, then all of row 1, and so on.
    std::size_t rowStart = static_cast<std::size_t>(cell.row) * static_cast<std::size_t>(columnCount);
    return rowStart + static_cast<std::size_t>(cell.column);
}

GridCell Grid::cellAt(std::size_t index) const
{
    if (index >= tiles.size())
    {
        throw GridBoundsError("Index " + std::to_string(index) + " is outside the grid's "
                              + std::to_string(tiles.size()) + " cells.");
    }

    GridCell cell;
    cell.column = static_cast<int>(index % static_cast<std::size_t>(columnCount));
    cell.row = static_cast<int>(index / static_cast<std::size_t>(columnCount));
    return cell;
}

const Tile& Grid::getTile(GridCell cell) const
{
    return tiles[indexOf(cell)];
}

void Grid::setTile(GridCell cell, TileType type)
{
    Tile& tile = tiles[indexOf(cell)];
    tile.type = type;
    if (type == TileType::SOFT_WALL)
    {
        tile.health = SoftWallMaxHealth;
        tile.intactPanels = AllPanelsIntact;
    }
    else
    {
        tile.health = 0;
        tile.intactPanels = 0;
    }
}

bool Grid::isWalkable(GridCell cell) const
{
    if (!contains(cell))
    {
        return false;
    }
    return getTile(cell).type == TileType::EMPTY;
}

bool Grid::blocksVision(GridCell cell) const
{
    // Everything outside the map blocks vision, so every ray is guaranteed to stop.
    if (!contains(cell))
    {
        return true;
    }
    return getTile(cell).type != TileType::EMPTY;
}

DamageResult Grid::damageTile(GridCell cell, std::uint16_t panelsHit)
{
    if (!contains(cell))
    {
        return DamageResult::NO_EFFECT;
    }

    Tile& tile = tiles[indexOf(cell)];
    if (tile.type != TileType::SOFT_WALL)
    {
        return DamageResult::NO_EFFECT;
    }

    // Health and panels are small integers. The arithmetic happens in int, so results are cast back explicitly.
    tile.health = static_cast<std::uint8_t>(tile.health - 1);
    tile.intactPanels = static_cast<std::uint16_t>(tile.intactPanels & ~panelsHit);
    if (tile.health == 0)
    {
        tile.type = TileType::EMPTY;
        tile.intactPanels = 0;
        return DamageResult::DESTROYED;
    }
    return DamageResult::DAMAGED;
}

std::uint16_t panelBit(int panelColumn, int panelRow)
{
    int bitIndex = panelRow * Grid::PanelsPerSide + panelColumn;
    return static_cast<std::uint16_t>(1u << bitIndex);
}

bool isPanelIntact(const Tile& tile, int panelColumn, int panelRow)
{
    return (tile.intactPanels & panelBit(panelColumn, panelRow)) != 0;
}

std::uint16_t panelsAlongShot(GridCell cell, sf::Vector2f shotFrom, sf::Vector2f shotAt)
{
    // The panel the shot lands on. Clamping keeps a hit on the tile's very edge inside the tile.
    float panelsPerTile = static_cast<float>(Grid::PanelsPerSide);
    int hitColumn = static_cast<int>(std::floor((shotAt.x - static_cast<float>(cell.column)) * panelsPerTile));
    int hitRow = static_cast<int>(std::floor((shotAt.y - static_cast<float>(cell.row)) * panelsPerTile));
    hitColumn = std::clamp(hitColumn, 0, Grid::PanelsPerSide - 1);
    hitRow = std::clamp(hitRow, 0, Grid::PanelsPerSide - 1);

    sf::Vector2f shot = shotAt - shotFrom;
    std::uint16_t panels = 0;
    if (std::abs(shot.x) >= std::abs(shot.y))
    {
        // Mostly horizontal: the slot is the whole row of panels at the height the shot hit.
        for (int panelColumn = 0; panelColumn < Grid::PanelsPerSide; ++panelColumn)
        {
            panels = static_cast<std::uint16_t>(panels | panelBit(panelColumn, hitRow));
        }
    }
    else
    {
        // Mostly vertical: the whole column of panels at the position the shot hit.
        for (int panelRow = 0; panelRow < Grid::PanelsPerSide; ++panelRow)
        {
            panels = static_cast<std::uint16_t>(panels | panelBit(hitColumn, panelRow));
        }
    }
    return panels;
}

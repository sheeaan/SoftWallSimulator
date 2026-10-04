#pragma once

#include <SFML/System/Vector2.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

enum class TileType : std::uint8_t
{
    EMPTY,
    SOFT_WALL,
    HARD_WALL
};

// Two bytes per tile, so the whole map is one small, contiguous block of memory.
struct Tile
{
    TileType type = TileType::EMPTY;
    std::uint8_t health = 0; // Hits left before a SOFT_WALL breaks. Other types ignore it.
};

struct GridCell
{
    int column = 0;
    int row = 0;
};

bool operator==(GridCell left, GridCell right);
bool operator!=(GridCell left, GridCell right);

enum class DamageResult
{
    NO_EFFECT, // The cell was floor, a HARD_WALL, or outside the grid.
    DAMAGED,   // A SOFT_WALL lost health but still stands.
    DESTROYED  // A SOFT_WALL reached zero health and became EMPTY.
};

// World positions are measured in tiles: cell (3, 2) covers x from 3 to 4 and y from 2 to 3.
GridCell cellContaining(sf::Vector2f worldPosition);
sf::Vector2f cellCenter(GridCell cell);

// The map: a 2D grid of tiles stored in one std::vector, row after row.
// Neighboring tiles in a row are neighbors in memory, so row-by-row loops stay in cache.
class Grid
{
public:
    static constexpr std::uint8_t SoftWallMaxHealth = 3;

    Grid(int columns, int rows);

    int getColumnCount() const;
    int getRowCount() const;
    std::size_t getCellCount() const;

    bool contains(GridCell cell) const;
    std::size_t indexOf(GridCell cell) const;
    GridCell cellAt(std::size_t index) const;

    const Tile& getTile(GridCell cell) const;
    void setTile(GridCell cell, TileType type);

    bool isWalkable(GridCell cell) const;
    bool blocksVision(GridCell cell) const;

    DamageResult damageTile(GridCell cell);

private:
    int columnCount;
    int rowCount;
    std::vector<Tile> tiles;
};

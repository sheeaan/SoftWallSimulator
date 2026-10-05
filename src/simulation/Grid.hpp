#pragma once

#include <SFML/System/Vector2.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

enum class TileType : std::uint8_t
{
    EMPTY,
    SOFT_WALL,       // Drywall. Bullets drill slots in it, and the third hit breaks it.
    HARD_WALL,       // Concrete. Nothing breaks it.
    REINFORCED_WALL, // Drywall the defender covered in steel. Only an exothermic charge breaks it.
    BARRICADE        // Wooden planks across a doorway. One hit breaks them, and a drone rolls under them.
};

// Four bytes per tile, so the whole map is one small, contiguous block of memory.
struct Tile
{
    TileType type = TileType::EMPTY;
    std::uint8_t health = 0;        // Hits left before a SOFT_WALL breaks. Other types ignore it.
    std::uint16_t intactPanels = 0; // One bit per panel of a SOFT_WALL's 4 x 4 panel grid. A cleared bit is a bullet hole.
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
    NO_EFFECT, // The cell was floor, a HARD_WALL, a REINFORCED_WALL, or outside the grid.
    DAMAGED,   // A SOFT_WALL lost health but still stands.
    DESTROYED  // A SOFT_WALL reached zero health, or a BARRICADE broke, and the cell became EMPTY.
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
    static constexpr int PanelsPerSide = 4; // A SOFT_WALL is a 4 x 4 grid of panels that bullets punch out.
    static constexpr std::uint16_t AllPanelsIntact = 0xFFFF;

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
    bool isDronePassable(GridCell cell) const; // Floor, or a barricade the drone rolls under.

    // Takes one health from a SOFT_WALL and punches out the panels the shot went through.
    // A BARRICADE breaks on the first hit.
    DamageResult damageTile(GridCell cell, std::uint16_t panelsHit);

    // Covers a SOFT_WALL in steel. Returns false when the cell is not a SOFT_WALL.
    bool reinforceTile(GridCell cell);

    // Blows a breakable tile (SOFT_WALL, REINFORCED_WALL, or BARRICADE) open. Returns false for anything else.
    bool destroyTile(GridCell cell);

    // Goes up by one every time any tile changes, so renderers know when to rebuild.
    unsigned int getRevision() const;

private:
    int columnCount;
    int rowCount;
    std::vector<Tile> tiles;
    unsigned int revision = 0;
};

// Panels are numbered row by row, like tiles: panel (column, row) is bit row * 4 + column.
std::uint16_t panelBit(int panelColumn, int panelRow);
bool isPanelIntact(const Tile& tile, int panelColumn, int panelRow);

// The panels a shot punches out of the wall in `cell`. The shot flies from shotFrom to shotAt and drills
// a straight slot, one panel wide, through the whole wall along its main axis: a row of panels for a
// mostly horizontal shot, a column for a mostly vertical one. Rays pass through the slot, so the player
// can see through the wall along the line of fire, but not from the side.
std::uint16_t panelsAlongShot(GridCell cell, sf::Vector2f shotFrom, sf::Vector2f shotAt);

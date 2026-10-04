#include "simulation/Raycaster.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr double Pi = 3.14159265358979323846;

    // A ray crosses at most every panel boundary of a tile once, so it leaves the tile within this many steps.
    constexpr int MaximumPanelSteps = 2 * Grid::PanelsPerSide + 1;
}

Raycaster::Raycaster()
{
    // The angles never change, so their sines and cosines are computed once, here.
    rayDirections.reserve(RayCount);
    for (int rayIndex = 0; rayIndex < RayCount; ++rayIndex)
    {
        double angle = 2.0 * Pi * static_cast<double>(rayIndex) / static_cast<double>(RayCount);
        rayDirections.push_back(sf::Vector2f(static_cast<float>(std::cos(angle)), static_cast<float>(std::sin(angle))));
    }
    visibilityHits.reserve(RayCount);
}

RayHit Raycaster::castRay(sf::Vector2f origin, sf::Vector2f direction, const Grid& grid) const
{
    const float infinity = std::numeric_limits<float>::infinity();
    GridCell cell = cellContaining(origin);

    // How far along the ray we travel to cross one whole column, or one whole row.
    float distancePerColumn = infinity;
    float distancePerRow = infinity;
    if (direction.x != 0.f)
    {
        distancePerColumn = std::abs(1.f / direction.x);
    }
    if (direction.y != 0.f)
    {
        distancePerRow = std::abs(1.f / direction.y);
    }

    // Which way the ray steps through columns, and how far it travels to reach the first column boundary.
    int columnStep = 0;
    float distanceToNextColumn = infinity;
    if (direction.x > 0.f)
    {
        columnStep = 1;
        distanceToNextColumn = (static_cast<float>(cell.column) + 1.f - origin.x) * distancePerColumn;
    }
    else if (direction.x < 0.f)
    {
        columnStep = -1;
        distanceToNextColumn = (origin.x - static_cast<float>(cell.column)) * distancePerColumn;
    }

    // The same for rows.
    int rowStep = 0;
    float distanceToNextRow = infinity;
    if (direction.y > 0.f)
    {
        rowStep = 1;
        distanceToNextRow = (static_cast<float>(cell.row) + 1.f - origin.y) * distancePerRow;
    }
    else if (direction.y < 0.f)
    {
        rowStep = -1;
        distanceToNextRow = (origin.y - static_cast<float>(cell.row)) * distancePerRow;
    }

    // Each step crosses whichever boundary is closer and enters the cell behind it.
    // A ray crosses each column and row boundary at most once, and cells outside the grid
    // block vision, so the ray always stops within this many steps.
    int maximumSteps = grid.getColumnCount() + grid.getRowCount() + 2;
    for (int step = 0; step < maximumSteps; ++step)
    {
        float crossingDistance = 0.f;
        if (distanceToNextColumn < distanceToNextRow)
        {
            crossingDistance = distanceToNextColumn;
            distanceToNextColumn += distancePerColumn;
            cell.column += columnStep;
        }
        else
        {
            crossingDistance = distanceToNextRow;
            distanceToNextRow += distancePerRow;
            cell.row += rowStep;
        }

        if (grid.blocksVision(cell))
        {
            // Usually the ray stops at the face it entered. Through a bullet hole, it keeps going.
            std::optional<float> stopDistance = findStopDistance(grid, cell, origin, direction, crossingDistance);
            if (stopDistance.has_value())
            {
                RayHit hit;
                hit.point = origin + direction * stopDistance.value();
                hit.distance = stopDistance.value();
                hit.cell = cell;
                return hit;
            }
        }
    }

    // Only a zero-length direction gets here: it never leaves its starting cell.
    RayHit stuckHit;
    stuckHit.point = origin;
    stuckHit.distance = 0.f;
    stuckHit.cell = cell;
    return stuckHit;
}

void Raycaster::castVisibility(sf::Vector2f origin, const Grid& grid)
{
    // clear() keeps the capacity, so after the first frame this never allocates.
    visibilityHits.clear();
    for (const sf::Vector2f& direction : rayDirections)
    {
        visibilityHits.push_back(castRay(origin, direction, grid));
    }
}

const std::vector<RayHit>& Raycaster::getVisibility() const
{
    return visibilityHits;
}

bool Raycaster::hasLineOfSight(sf::Vector2f from, sf::Vector2f to, const Grid& grid) const
{
    sf::Vector2f offset = to - from;
    float distance = offset.length();
    if (distance == 0.f)
    {
        return true;
    }

    // The target is visible when the first blocking cell lies beyond it.
    RayHit hit = castRay(from, offset / distance, grid);
    return hit.distance >= distance;
}

std::optional<float> Raycaster::findStopDistance(const Grid& grid, GridCell cell, sf::Vector2f origin,
                                                 sf::Vector2f direction, float entryDistance) const
{
    // Hard walls, intact soft walls, and cells outside the map stop the ray where it enters them.
    if (!grid.contains(cell))
    {
        return entryDistance;
    }
    const Tile& tile = grid.getTile(cell);
    if (tile.type != TileType::SOFT_WALL || tile.intactPanels == Grid::AllPanelsIntact)
    {
        return entryDistance;
    }

    // A soft wall with bullet holes: run the same DDA again inside the tile, on its 4 x 4 panel grid.
    // The ray stops at the first intact panel. If it leaves the tile first, it went through the holes.
    const float infinity = std::numeric_limits<float>::infinity();
    const float panelSize = 1.f / static_cast<float>(Grid::PanelsPerSide);
    sf::Vector2f tileCorner(static_cast<float>(cell.column), static_cast<float>(cell.row));
    sf::Vector2f entryPoint = origin + direction * entryDistance;

    // The entry point lies on the tile's edge, so clamping keeps rounding from placing it in a neighbor.
    int panelColumn = static_cast<int>(std::floor((entryPoint.x - tileCorner.x) / panelSize));
    int panelRow = static_cast<int>(std::floor((entryPoint.y - tileCorner.y) / panelSize));
    panelColumn = std::clamp(panelColumn, 0, Grid::PanelsPerSide - 1);
    panelRow = std::clamp(panelRow, 0, Grid::PanelsPerSide - 1);

    // The same bookkeeping as the tile DDA, with panel-sized cells.
    int columnStep = 0;
    float distancePerPanelColumn = infinity;
    float distanceToNextPanelColumn = infinity;
    if (direction.x > 0.f)
    {
        columnStep = 1;
        distancePerPanelColumn = panelSize / direction.x;
        float nextBoundary = tileCorner.x + static_cast<float>(panelColumn + 1) * panelSize;
        distanceToNextPanelColumn = (nextBoundary - origin.x) / direction.x;
    }
    else if (direction.x < 0.f)
    {
        columnStep = -1;
        distancePerPanelColumn = panelSize / -direction.x;
        float nextBoundary = tileCorner.x + static_cast<float>(panelColumn) * panelSize;
        distanceToNextPanelColumn = (nextBoundary - origin.x) / direction.x;
    }

    int rowStep = 0;
    float distancePerPanelRow = infinity;
    float distanceToNextPanelRow = infinity;
    if (direction.y > 0.f)
    {
        rowStep = 1;
        distancePerPanelRow = panelSize / direction.y;
        float nextBoundary = tileCorner.y + static_cast<float>(panelRow + 1) * panelSize;
        distanceToNextPanelRow = (nextBoundary - origin.y) / direction.y;
    }
    else if (direction.y < 0.f)
    {
        rowStep = -1;
        distancePerPanelRow = panelSize / -direction.y;
        float nextBoundary = tileCorner.y + static_cast<float>(panelRow) * panelSize;
        distanceToNextPanelRow = (nextBoundary - origin.y) / direction.y;
    }

    float distance = entryDistance;
    for (int step = 0; step < MaximumPanelSteps; ++step)
    {
        bool insideTile = panelColumn >= 0 && panelColumn < Grid::PanelsPerSide && panelRow >= 0
                          && panelRow < Grid::PanelsPerSide;
        if (!insideTile)
        {
            return std::nullopt;
        }
        if (isPanelIntact(tile, panelColumn, panelRow))
        {
            return distance;
        }

        if (distanceToNextPanelColumn < distanceToNextPanelRow)
        {
            distance = distanceToNextPanelColumn;
            distanceToNextPanelColumn += distancePerPanelColumn;
            panelColumn += columnStep;
        }
        else
        {
            distance = distanceToNextPanelRow;
            distanceToNextPanelRow += distancePerPanelRow;
            panelRow += rowStep;
        }
    }

    // Only a zero-length direction gets here: it never leaves its panel.
    return distance;
}

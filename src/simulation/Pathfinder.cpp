#include "simulation/Pathfinder.hpp"

#include "simulation/SimulatorErrors.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <string>

namespace
{
    constexpr int Unreached = std::numeric_limits<int>::max();
    constexpr std::size_t NoCell = std::numeric_limits<std::size_t>::max();

    constexpr GridCell NeighborOffsets[8] = {
        GridCell{1, 0}, GridCell{-1, 0}, GridCell{0, 1},  GridCell{0, -1},
        GridCell{1, 1}, GridCell{1, -1}, GridCell{-1, 1}, GridCell{-1, -1},
    };
}

int Pathfinder::estimateCost(GridCell from, GridCell to)
{
    // Take as many diagonal steps as the shorter axis allows, then go straight for the rest.
    int columnDistance = std::abs(from.column - to.column);
    int rowDistance = std::abs(from.row - to.row);
    int diagonalSteps = std::min(columnDistance, rowDistance);
    int straightSteps = std::max(columnDistance, rowDistance) - diagonalSteps;
    return diagonalSteps * DiagonalStepCost + straightSteps * StraightStepCost;
}

bool Pathfinder::OpenEntryIsWorse::operator()(const OpenEntry& left, const OpenEntry& right) const
{
    if (left.estimatedTotalCost != right.estimatedTotalCost)
    {
        return left.estimatedTotalCost > right.estimatedTotalCost;
    }
    return left.estimatedRemainingCost > right.estimatedRemainingCost;
}

std::vector<GridCell> Pathfinder::findPath(const Grid& grid, GridCell start, GridCell goal)
{
    // Reset the bookkeeping. assign() reuses the memory once the arrays have grown to the grid's size.
    lastStart = start;
    lastGoal = goal;
    std::size_t cellCount = grid.getCellCount();
    costFromStart.assign(cellCount, Unreached);
    cameFrom.assign(cellCount, NoCell);
    searchStates.assign(cellCount, SearchState::UNVISITED);
    openHeap.clear();
    expandedCells.clear();

    std::vector<GridCell> path;
    if (!grid.isWalkable(start) || !grid.isWalkable(goal))
    {
        return path;
    }

    std::size_t startIndex = grid.indexOf(start);
    std::size_t goalIndex = grid.indexOf(goal);

    OpenEntry startEntry;
    startEntry.cellIndex = startIndex;
    startEntry.estimatedRemainingCost = estimateCost(start, goal);
    startEntry.estimatedTotalCost = startEntry.estimatedRemainingCost;
    costFromStart[startIndex] = 0;
    searchStates[startIndex] = SearchState::OPEN;
    openHeap.push_back(startEntry);
    std::push_heap(openHeap.begin(), openHeap.end(), OpenEntryIsWorse());

    while (!openHeap.empty())
    {
        std::pop_heap(openHeap.begin(), openHeap.end(), OpenEntryIsWorse());
        OpenEntry current = openHeap.back();
        openHeap.pop_back();

        // A cell can be in the heap more than once. Only its first, cheapest entry is expanded.
        if (searchStates[current.cellIndex] == SearchState::CLOSED)
        {
            continue;
        }
        searchStates[current.cellIndex] = SearchState::CLOSED;
        expandedCells.push_back(current.cellIndex);

        if (current.cellIndex == goalIndex)
        {
            break;
        }

        GridCell currentCell = grid.cellAt(current.cellIndex);
        for (const GridCell& offset : NeighborOffsets)
        {
            GridCell neighbor{currentCell.column + offset.column, currentCell.row + offset.row};
            if (!grid.isWalkable(neighbor))
            {
                continue;
            }

            int stepCost = StraightStepCost;
            bool isDiagonal = offset.column != 0 && offset.row != 0;
            if (isDiagonal)
            {
                // No corner cutting: both cells beside a diagonal step must be open,
                // otherwise the enemy would clip the corner of a wall.
                bool horizontalSideOpen = grid.isWalkable(GridCell{currentCell.column + offset.column, currentCell.row});
                bool verticalSideOpen = grid.isWalkable(GridCell{currentCell.column, currentCell.row + offset.row});
                if (!horizontalSideOpen || !verticalSideOpen)
                {
                    continue;
                }
                stepCost = DiagonalStepCost;
            }

            std::size_t neighborIndex = grid.indexOf(neighbor);
            if (searchStates[neighborIndex] == SearchState::CLOSED)
            {
                continue;
            }

            int newCost = costFromStart[current.cellIndex] + stepCost;
            if (newCost < costFromStart[neighborIndex])
            {
                costFromStart[neighborIndex] = newCost;
                cameFrom[neighborIndex] = current.cellIndex;
                searchStates[neighborIndex] = SearchState::OPEN;

                OpenEntry entry;
                entry.cellIndex = neighborIndex;
                entry.estimatedRemainingCost = estimateCost(neighbor, goal);
                entry.estimatedTotalCost = newCost + entry.estimatedRemainingCost;
                openHeap.push_back(entry);
                std::push_heap(openHeap.begin(), openHeap.end(), OpenEntryIsWorse());
            }
        }
    }

    if (searchStates[goalIndex] != SearchState::CLOSED)
    {
        return path;
    }

    // Walk back from the goal to the start, then reverse into start-to-goal order.
    std::size_t cellIndex = goalIndex;
    while (cellIndex != NoCell)
    {
        path.push_back(grid.cellAt(cellIndex));
        cellIndex = cameFrom[cellIndex];
    }
    std::reverse(path.begin(), path.end());
    return path;
}

const std::vector<std::size_t>& Pathfinder::getExpandedCells() const
{
    return expandedCells;
}

GridCell Pathfinder::getStart() const
{
    return lastStart;
}

GridCell Pathfinder::getGoal() const
{
    return lastGoal;
}

bool Pathfinder::wasExpanded(std::size_t cellIndex) const
{
    checkIndex(cellIndex);
    return searchStates[cellIndex] == SearchState::CLOSED;
}

int Pathfinder::getCostFromStart(std::size_t cellIndex) const
{
    checkIndex(cellIndex);
    return costFromStart[cellIndex];
}

std::optional<std::size_t> Pathfinder::getCameFrom(std::size_t cellIndex) const
{
    checkIndex(cellIndex);
    if (cameFrom[cellIndex] == NoCell)
    {
        return std::nullopt;
    }
    return cameFrom[cellIndex];
}

void Pathfinder::checkIndex(std::size_t cellIndex) const
{
    if (cellIndex >= searchStates.size())
    {
        throw GridBoundsError("Cell index " + std::to_string(cellIndex) + " is outside the last search's "
                              + std::to_string(searchStates.size()) + " cells.");
    }
}

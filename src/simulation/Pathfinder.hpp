#pragma once

#include "simulation/Grid.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

// A* over the grid in 8 directions, never cutting a wall's corner.
// The per-cell bookkeeping lives in flat arrays indexed like the grid's tiles. The arrays are
// reused from one search to the next, so after the first search only the returned path allocates.
class Pathfinder
{
public:
    static constexpr int StraightStepCost = 10;
    static constexpr int DiagonalStepCost = 14; // 10 times the square root of 2, rounded, so costs stay integers.

    // Octile distance: the exact cost between two cells on an open grid. It never overestimates,
    // so A* stays optimal. This is h.
    static int estimateCost(GridCell from, GridCell to);

    // Returns the cells from start to goal, both included, along a cheapest path.
    // Returns an empty path when the goal cannot be reached.
    std::vector<GridCell> findPath(const Grid& grid, GridCell start, GridCell goal);

    // What the last search did, for the math view.
    const std::vector<std::size_t>& getExpandedCells() const; // Cell indices, in expansion order.
    GridCell getStart() const;
    GridCell getGoal() const;
    bool wasExpanded(std::size_t cellIndex) const;
    int getCostFromStart(std::size_t cellIndex) const;                  // g. Only meaningful for expanded cells.
    std::optional<std::size_t> getCameFrom(std::size_t cellIndex) const; // The parent. Empty for the start.

private:
    enum class SearchState : std::uint8_t
    {
        UNVISITED,
        OPEN,
        CLOSED
    };

    struct OpenEntry
    {
        std::size_t cellIndex = 0;
        int estimatedTotalCost = 0;     // Cost so far plus the estimate to the goal.
        int estimatedRemainingCost = 0; // The estimate to the goal alone.
    };

    // Orders the heap so the entry with the lowest estimated total comes out first.
    // Ties go to the entry closer to the goal, which keeps the search moving forward.
    struct OpenEntryIsWorse
    {
        bool operator()(const OpenEntry& left, const OpenEntry& right) const;
    };

    void checkIndex(std::size_t cellIndex) const;

    std::vector<int> costFromStart;         // Per cell: the cheapest known cost from the start.
    std::vector<std::size_t> cameFrom;      // Per cell: the previous cell on that cheapest path.
    std::vector<SearchState> searchStates;  // Per cell.
    std::vector<OpenEntry> openHeap;        // A binary heap, kept with std::push_heap and std::pop_heap.
    std::vector<std::size_t> expandedCells; // In expansion order.
    GridCell lastStart;
    GridCell lastGoal;
};

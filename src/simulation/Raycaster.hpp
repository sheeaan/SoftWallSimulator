#pragma once

#include "simulation/Grid.hpp"

#include <SFML/System/Vector2.hpp>

#include <optional>
#include <vector>

struct RayHit
{
    sf::Vector2f point;    // Where the ray stopped, in tiles.
    float distance = 0.f;  // From the ray's origin, in tiles.
    GridCell cell;         // The non-EMPTY cell that stopped the ray.
};

// One DDA step, recorded for the math view. Distances are measured along the ray, in tiles.
struct DdaStep
{
    float distanceToNextColumn = 0.f; // tMaxX before the step: how far to the next column boundary.
    float distanceToNextRow = 0.f;    // tMaxY before the step: how far to the next row boundary.
    bool crossedColumn = false;       // True when tMaxX was smaller, so the ray crossed into the next column.
    float crossingDistance = 0.f;     // The smaller of the two: where the ray crossed.
    GridCell cell;                    // The cell the ray entered.
};

// Everything the DDA computed for one ray.
struct RayTrace
{
    sf::Vector2f origin;
    sf::Vector2f direction;
    float distancePerColumn = 0.f; // tDeltaX: ray length that crosses one whole column, 1 / |dx|.
    float distancePerRow = 0.f;    // tDeltaY: ray length that crosses one whole row, 1 / |dy|.
    std::vector<DdaStep> steps;
    RayHit hit;
};

// Casts rays through the grid with DDA, the Digital Differential Analyzer
// (Amanatides and Woo, "A Fast Voxel Traversal Algorithm", 1987).
// A ray visits exactly the cells it crosses, in order, and stops at the first one that blocks vision.
// Inside a soft wall with bullet holes, a second, finer DDA walks the wall's panels,
// so a ray can pass through a hole.
class Raycaster
{
public:
    static constexpr int RayCount = 360;

    Raycaster();

    // When stepLog is given, every DDA step is appended to it.
    RayHit castRay(sf::Vector2f origin, sf::Vector2f direction, const Grid& grid,
                   std::vector<DdaStep>* stepLog = nullptr) const;

    // Casts one ray and keeps every number the DDA computed along the way.
    RayTrace traceRay(sf::Vector2f origin, sf::Vector2f direction, const Grid& grid) const;

    // Casts RayCount rays evenly around the origin, one per degree, in angle order.
    void castVisibility(sf::Vector2f origin, const Grid& grid);
    const std::vector<RayHit>& getVisibility() const;

    bool hasLineOfSight(sf::Vector2f from, sf::Vector2f to, const Grid& grid) const;

private:
    // How far along the ray a blocking cell stops it. Empty when the ray passes through the cell's bullet holes.
    std::optional<float> findStopDistance(const Grid& grid, GridCell cell, sf::Vector2f origin,
                                          sf::Vector2f direction, float entryDistance) const;

    std::vector<sf::Vector2f> rayDirections; // RayCount unit vectors, computed once.
    std::vector<RayHit> visibilityHits;      // Refilled every frame without reallocating.
};

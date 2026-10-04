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

    RayHit castRay(sf::Vector2f origin, sf::Vector2f direction, const Grid& grid) const;

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

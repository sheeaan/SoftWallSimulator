#pragma once

#include "simulation/World.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>

// Tab toggles a view of the algorithms at work: the 360 rays and the polygon they form,
// the cells the last A* search expanded (colored by expansion order), and the enemy's path.
class AlgorithmOverlay
{
public:
    AlgorithmOverlay();

    void toggle();
    bool isVisible() const;

    // Makes the path flash, to show that a destroyed wall forced a re-plan.
    void flashPath();

    void update(float seconds);
    void draw(sf::RenderTarget& target, const World& world);

private:
    void buildExploredCells(const World& world);
    void buildRays(const World& world);
    void buildPath(const World& world);

    bool visible = false;
    float elapsedSeconds = 0.f;
    float flashSecondsLeft = 0.f;
    sf::VertexArray exploredVertices;
    sf::VertexArray rayVertices;
    sf::VertexArray outlineVertices;
    sf::VertexArray pathLineVertices;
    sf::VertexArray pathDotVertices;
};

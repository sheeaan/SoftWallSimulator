#pragma once

#include "simulation/World.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>

// Draws the attacker's gadgets and what they reveal: charges counting down on walls, the drone,
// the bomb site's label, and the orange mark on a spotted defender, which shows through walls.
// Everything here is drawn after the light map, so it stays visible in the dark.
class GadgetRenderer
{
public:
    GadgetRenderer(sf::Vector2u windowSize, const Grid& grid, const std::vector<GridCell>& bombSite);

    // The label points at the font member, so a copy would point at the original's font.
    GadgetRenderer(const GadgetRenderer&) = delete;
    GadgetRenderer& operator=(const GadgetRenderer&) = delete;

    void draw(sf::RenderTarget& target, const World& world, float elapsedSeconds);

private:
    void appendCharge(const Charge& charge, float elapsedSeconds);

    sf::Font font;
    sf::Text siteLabel;
    bool hasBombSite;
    sf::VertexArray chargeShapes;
    sf::CircleShape droneBody;
    sf::CircleShape droneSignal;
    sf::CircleShape spottedMarker;
};

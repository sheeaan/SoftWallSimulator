#pragma once

#include "simulation/World.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>

// A panel in the top-left corner with live numbers from the simulation and the controls.
// The text refreshes four times a second, so the numbers stay readable.
class StatisticsPanel
{
public:
    explicit StatisticsPanel(sf::Vector2u windowSize);

    // The texts point at the font member, so a copy would point at the original's font.
    StatisticsPanel(const StatisticsPanel&) = delete;
    StatisticsPanel& operator=(const StatisticsPanel&) = delete;

    void update(float seconds, const World& world);
    void draw(sf::RenderTarget& target) const;

private:
    sf::Font font;
    sf::Text titleText;
    sf::Text bodyText;
    sf::RectangleShape background;
    sf::RectangleShape accentBar;
    float padding;
    int framesSinceRefresh = 0;
    float secondsSinceRefresh = 0.f;
};

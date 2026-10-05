#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>

#include <vector>

// The Siege-style screen overlay: an attacker-versus-defender banner at the top center,
// and key prompts along the bottom. It is laid out once, in pixels, when the window opens.
class HeadsUpDisplay
{
public:
    explicit HeadsUpDisplay(sf::Vector2u windowSize);

    // The texts point at the font member, so a copy would point at the original's font.
    HeadsUpDisplay(const HeadsUpDisplay&) = delete;
    HeadsUpDisplay& operator=(const HeadsUpDisplay&) = delete;

    // Shows whether the player can see the defender right now.
    void setEnemySpotted(bool spotted);
    void draw(sf::RenderTarget& target) const;

private:
    sf::Font font;
    float unit; // One percent of the window height. Every size is a multiple of it.

    sf::RectangleShape attackerBlock;
    sf::RectangleShape statusBlock;
    sf::RectangleShape defenderBlock;
    sf::Text attackerText;
    sf::Text statusText;
    sf::Text defenderText;
    bool enemySpotted = false;

    std::vector<sf::RectangleShape> keycaps;
    std::vector<sf::Text> keyTexts;
    std::vector<sf::Text> actionTexts;
};

#pragma once

#include "simulation/World.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <string>
#include <vector>

// The Siege-style screen overlay: the operator banner and round clock at the top center, flanked by the
// official Striker and Sentry icons, the spotted and planting status under it, short messages, the round
// result with the winner's icon, and key prompts along the bottom. It is laid out in pixels.
class HeadsUpDisplay
{
public:
    explicit HeadsUpDisplay(sf::Vector2u windowPixelSize);

    // The texts and sprites point at the font and texture members, so a copy would point at the original's.
    HeadsUpDisplay(const HeadsUpDisplay&) = delete;
    HeadsUpDisplay& operator=(const HeadsUpDisplay&) = delete;

    void update(const World& world, float seconds);

    // Shows a short message under the banner for a couple of seconds.
    void showMessage(const std::string& message);

    void draw(sf::RenderTarget& target) const;

private:
    void layOutPrompts(int breachChargesLeft, int hardBreachChargesLeft);

    sf::Font font;
    sf::Texture strikerBadge; // Official operator icons from Ubisoft's fan kit.
    sf::Texture sentryBadge;
    sf::Vector2f windowSize;
    float unit; // One percent of the window height. Every size is a multiple of it.

    sf::RectangleShape attackerBlock;
    sf::RectangleShape clockBlock;
    sf::RectangleShape defenderBlock;
    sf::Sprite strikerSprite;
    sf::Sprite sentrySprite;
    sf::Text attackerText;
    sf::Text clockText;
    sf::Text defenderText;
    sf::Text statusText;
    sf::RectangleShape plantBarBackground;
    sf::RectangleShape plantBarFill;
    bool planting = false;
    sf::Text messageText;
    float messageSecondsLeft = 0.f;

    bool roundOver = false;
    sf::RectangleShape resultBackground;
    sf::Sprite resultBadge;
    sf::Text resultTitle;
    sf::Text resultReason;
    sf::Text resultHint;

    int promptedBreachCharges = -1;
    int promptedHardBreachCharges = -1;
    std::vector<sf::RectangleShape> keycaps;
    std::vector<sf::Text> keyTexts;
    std::vector<sf::Text> actionTexts;
};

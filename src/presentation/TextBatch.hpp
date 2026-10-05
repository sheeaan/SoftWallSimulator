#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>

enum class TextAlignment
{
    LEFT,
    CENTER,
    RIGHT
};

// Builds many short labels into one vertex array, so hundreds of numbers draw in a single call.
// sf::Text would cost one draw call per label. Positions are in world units: each glyph's pixel
// size is scaled by worldUnitsPerPixel, so labels stay the same size on screen as plain text.
class TextBatch
{
public:
    // The font must outlive the batch.
    TextBatch(const sf::Font& labelFont, unsigned int labelCharacterSize, float labelWorldUnitsPerPixel);

    void clear();

    // Adds one ASCII label. The anchor is the label's vertical middle, at its left edge, center, or right edge.
    void append(const std::string& text, sf::Vector2f anchor, TextAlignment alignment, sf::Color color);

    void draw(sf::RenderTarget& target) const;

private:
    const sf::Font& font;
    unsigned int characterSize;
    float worldUnitsPerPixel;
    sf::VertexArray vertices;
};

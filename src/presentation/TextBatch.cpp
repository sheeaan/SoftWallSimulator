#include "presentation/TextBatch.hpp"

#include "presentation/VertexQuads.hpp"

#include <SFML/Graphics/Glyph.hpp>
#include <SFML/Graphics/RenderStates.hpp>

namespace
{
    constexpr float GlyphPadding = 1.f; // Pixels around each glyph, as sf::Text uses, so smoothing never clips an edge.

    char32_t toCodePoint(char character)
    {
        return static_cast<char32_t>(static_cast<unsigned char>(character));
    }
}

TextBatch::TextBatch(const sf::Font& labelFont, unsigned int labelCharacterSize, float labelWorldUnitsPerPixel)
    : font(labelFont)
    , characterSize(labelCharacterSize)
    , worldUnitsPerPixel(labelWorldUnitsPerPixel)
    , vertices(sf::PrimitiveType::Triangles)
{
}

void TextBatch::clear()
{
    vertices.clear();
}

void TextBatch::append(const std::string& text, sf::Vector2f anchor, TextAlignment alignment, sf::Color color)
{
    // Measure the label: the sum of its glyph advances.
    float widthInPixels = 0.f;
    for (char character : text)
    {
        widthInPixels += font.getGlyph(toCodePoint(character), characterSize, false).advance;
    }
    float width = widthInPixels * worldUnitsPerPixel;

    float penX = anchor.x;
    if (alignment == TextAlignment::CENTER)
    {
        penX -= width / 2.f;
    }
    else if (alignment == TextAlignment::RIGHT)
    {
        penX -= width;
    }

    // Digits rise about 0.7 of the character size above the baseline,
    // so a baseline 0.35 below the anchor centers them on it.
    float baselineY = anchor.y + 0.35f * static_cast<float>(characterSize) * worldUnitsPerPixel;

    for (char character : text)
    {
        const sf::Glyph& glyph = font.getGlyph(toCodePoint(character), characterSize, false);

        sf::Vector2f pixelTopLeft = glyph.bounds.position - sf::Vector2f(GlyphPadding, GlyphPadding);
        sf::Vector2f pixelSize = glyph.bounds.size + sf::Vector2f(GlyphPadding * 2.f, GlyphPadding * 2.f);
        sf::Vector2f textureTopLeft = sf::Vector2f(glyph.textureRect.position) - sf::Vector2f(GlyphPadding, GlyphPadding);
        sf::Vector2f textureSize = sf::Vector2f(glyph.textureRect.size) + sf::Vector2f(GlyphPadding * 2.f, GlyphPadding * 2.f);

        sf::Vector2f topLeft(penX + pixelTopLeft.x * worldUnitsPerPixel, baselineY + pixelTopLeft.y * worldUnitsPerPixel);
        appendTexturedQuad(vertices, topLeft, pixelSize * worldUnitsPerPixel, textureTopLeft, textureSize, color);

        penX += glyph.advance * worldUnitsPerPixel;
    }
}

void TextBatch::draw(sf::RenderTarget& target) const
{
    // Every glyph of one character size lives on the same font texture page.
    sf::RenderStates states;
    states.texture = &font.getTexture(characterSize);
    target.draw(vertices, states);
}

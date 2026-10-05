#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

// Appends two triangles that cover the axis-aligned rectangle from topLeft to bottomRight.
// Every renderer batches its rectangles this way, so each layer is one draw call.
inline void appendQuad(sf::VertexArray& vertices, sf::Vector2f topLeft, sf::Vector2f bottomRight, sf::Color color)
{
    sf::Vector2f topRight(bottomRight.x, topLeft.y);
    sf::Vector2f bottomLeft(topLeft.x, bottomRight.y);

    vertices.append(sf::Vertex{topLeft, color});
    vertices.append(sf::Vertex{topRight, color});
    vertices.append(sf::Vertex{bottomRight, color});

    vertices.append(sf::Vertex{topLeft, color});
    vertices.append(sf::Vertex{bottomRight, color});
    vertices.append(sf::Vertex{bottomLeft, color});
}

// The same two triangles, textured: textureTopLeft and textureSize pick a rectangle of the texture, in pixels.
inline void appendTexturedQuad(sf::VertexArray& vertices, sf::Vector2f topLeft, sf::Vector2f size,
                               sf::Vector2f textureTopLeft, sf::Vector2f textureSize, sf::Color color)
{
    sf::Vector2f topRight = topLeft + sf::Vector2f(size.x, 0.f);
    sf::Vector2f bottomRight = topLeft + size;
    sf::Vector2f bottomLeft = topLeft + sf::Vector2f(0.f, size.y);
    sf::Vector2f textureTopRight = textureTopLeft + sf::Vector2f(textureSize.x, 0.f);
    sf::Vector2f textureBottomRight = textureTopLeft + textureSize;
    sf::Vector2f textureBottomLeft = textureTopLeft + sf::Vector2f(0.f, textureSize.y);

    vertices.append(sf::Vertex{topLeft, color, textureTopLeft});
    vertices.append(sf::Vertex{topRight, color, textureTopRight});
    vertices.append(sf::Vertex{bottomRight, color, textureBottomRight});

    vertices.append(sf::Vertex{topLeft, color, textureTopLeft});
    vertices.append(sf::Vertex{bottomRight, color, textureBottomRight});
    vertices.append(sf::Vertex{bottomLeft, color, textureBottomLeft});
}

// A straight line with thickness, as two triangles. The corners sit half the thickness away
// from the line along its normal, which is the line's direction turned 90 degrees.
inline void appendLine(sf::VertexArray& vertices, sf::Vector2f from, sf::Vector2f to, float thickness, sf::Color color)
{
    sf::Vector2f along = to - from;
    float length = along.length();
    if (length == 0.f)
    {
        return;
    }
    sf::Vector2f halfWidth = along.perpendicular() / length * (thickness / 2.f);

    vertices.append(sf::Vertex{from + halfWidth, color});
    vertices.append(sf::Vertex{to + halfWidth, color});
    vertices.append(sf::Vertex{to - halfWidth, color});

    vertices.append(sf::Vertex{from + halfWidth, color});
    vertices.append(sf::Vertex{to - halfWidth, color});
    vertices.append(sf::Vertex{from - halfWidth, color});
}

// The border of a rectangle, as four thin quads that stay inside it.
inline void appendOutline(sf::VertexArray& vertices, sf::Vector2f topLeft, sf::Vector2f bottomRight, float thickness,
                          sf::Color color)
{
    appendQuad(vertices, topLeft, sf::Vector2f(bottomRight.x, topLeft.y + thickness), color);
    appendQuad(vertices, sf::Vector2f(topLeft.x, bottomRight.y - thickness), bottomRight, color);
    appendQuad(vertices, sf::Vector2f(topLeft.x, topLeft.y + thickness), sf::Vector2f(topLeft.x + thickness, bottomRight.y - thickness), color);
    appendQuad(vertices, sf::Vector2f(bottomRight.x - thickness, topLeft.y + thickness), sf::Vector2f(bottomRight.x, bottomRight.y - thickness), color);
}

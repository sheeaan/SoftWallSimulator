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

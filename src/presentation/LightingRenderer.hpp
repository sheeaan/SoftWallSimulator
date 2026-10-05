#pragma once

#include "simulation/Grid.hpp"
#include "simulation/Raycaster.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/View.hpp>

#include <vector>

// Turns ray hits into light.
// For each light, the hits, in angle order, form a triangle fan around its origin: the visibility polygon.
// The fans are added into a light map with a radial gradient, on top of a dim ambient level,
// and the light map is multiplied over the scene. Whatever no ray reaches falls into darkness.
class LightingRenderer
{
public:
    explicit LightingRenderer(sf::Vector2u windowSize);

    // Starts a new light map at the ambient level, seen through the world view.
    void beginLightMap(const sf::View& worldView);

    // Adds one light: its visibility polygon, fading to nothing at `radius` tiles, and the wall faces its rays hit.
    void addLight(const Grid& grid, sf::Vector2f origin, const std::vector<RayHit>& visibility, float radius,
                  sf::Color color);

    // Multiplies the finished light map over everything already drawn to the target.
    void applyLightMap(sf::RenderTarget& target);

    // The same soft radial gradient, reused for the glows around the player and the enemy.
    const sf::Texture& getGlowTexture() const;

private:
    sf::RenderTexture lightMap;
    sf::Texture radialGradient;
    sf::VertexArray visibilityFan;
    sf::VertexArray wallLightVertices;
    std::vector<float> nearestHitDistances; // Per cell, refilled for every light without reallocating.
};

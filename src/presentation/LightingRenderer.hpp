#pragma once

#include "simulation/Grid.hpp"
#include "simulation/Raycaster.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/View.hpp>

#include <vector>

// Turns the ray hits into light.
// The hits, in angle order, form a triangle fan around the player: the visibility polygon.
// The fan is drawn into a light map with a radial gradient, on top of a dim ambient level,
// and the light map is multiplied over the scene. Whatever the rays cannot reach falls into darkness.
class LightingRenderer
{
public:
    static constexpr float LightRadius = 16.f; // Tiles. The light fades to the ambient level at this distance.

    explicit LightingRenderer(sf::Vector2u windowSize);

    void drawLightMap(sf::RenderTarget& target,
                      const sf::View& worldView,
                      const Grid& grid,
                      sf::Vector2f lightOrigin,
                      const std::vector<RayHit>& visibility);

    // The same soft radial gradient, reused for the glows around the player and the enemy.
    const sf::Texture& getGlowTexture() const;

private:
    sf::RenderTexture lightMap;
    sf::Texture radialGradient;
    sf::VertexArray visibilityFan;
    sf::VertexArray wallLightVertices;
    std::vector<float> nearestHitDistances; // Per cell, refilled every frame without reallocating.
};

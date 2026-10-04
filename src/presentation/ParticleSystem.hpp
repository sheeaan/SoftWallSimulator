#pragma once

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

#include <cstddef>
#include <random>
#include <vector>

struct Particle
{
    sf::Vector2f position;
    sf::Vector2f velocity;     // Tiles per second.
    float size = 0.f;          // Tiles.
    float lifetimeSeconds = 0.f;
    float secondsLeft = 0.f;
    sf::Color color;
};

// A fixed-capacity pool of short-lived square particles, drawn from one vertex array.
// The pool reserves its memory up front. A dead particle is replaced by the last one,
// so removal is constant time and the pool never reallocates.
class ParticleSystem
{
public:
    explicit ParticleSystem(std::size_t maximumParticles);

    // Spawns particles that fly outward from a point in random directions. Spawns beyond capacity are dropped.
    void spawnBurst(sf::Vector2f center, int count, sf::Color color, float speed, float lifetimeSeconds, float size);

    void update(float seconds);
    void draw(sf::RenderTarget& target, sf::BlendMode blendMode);

private:
    std::size_t capacity;
    std::vector<Particle> particles;
    sf::VertexArray vertices;
    std::mt19937 randomGenerator;
};

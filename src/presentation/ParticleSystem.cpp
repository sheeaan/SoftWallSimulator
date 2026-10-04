#include "presentation/ParticleSystem.hpp"

#include "presentation/VertexQuads.hpp"

#include <SFML/Graphics/RenderStates.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace
{
    constexpr float Pi = 3.14159265f;
    constexpr float FrictionPerSecond = 3.f; // How quickly particles lose their speed.
}

ParticleSystem::ParticleSystem(std::size_t maximumParticles)
    : capacity(maximumParticles)
    , vertices(sf::PrimitiveType::Triangles)
    , randomGenerator(2024) // A fixed seed: the effect only needs to look random.
{
    particles.reserve(capacity);
}

void ParticleSystem::spawnBurst(sf::Vector2f center, int count, sf::Color color, float speed, float lifetimeSeconds, float size)
{
    std::uniform_real_distribution<float> angleDistribution(0.f, 2.f * Pi);
    std::uniform_real_distribution<float> variationDistribution(0.4f, 1.f);

    for (int spawned = 0; spawned < count; ++spawned)
    {
        if (particles.size() >= capacity)
        {
            return;
        }

        float angle = angleDistribution(randomGenerator);
        Particle particle;
        particle.position = center;
        particle.velocity = sf::Vector2f(std::cos(angle), std::sin(angle)) * (speed * variationDistribution(randomGenerator));
        particle.size = size * variationDistribution(randomGenerator);
        particle.lifetimeSeconds = lifetimeSeconds * variationDistribution(randomGenerator);
        particle.secondsLeft = particle.lifetimeSeconds;
        particle.color = color;
        particles.push_back(particle);
    }
}

void ParticleSystem::update(float seconds)
{
    float speedKept = std::max(0.f, 1.f - FrictionPerSecond * seconds);

    std::size_t index = 0;
    while (index < particles.size())
    {
        Particle& particle = particles[index];
        particle.secondsLeft -= seconds;
        if (particle.secondsLeft <= 0.f)
        {
            // Swap and pop: move the last particle into this slot. Do not advance the index,
            // because the moved particle has not been updated yet.
            particles[index] = particles.back();
            particles.pop_back();
            continue;
        }

        particle.position += particle.velocity * seconds;
        particle.velocity *= speedKept;
        index += 1;
    }
}

void ParticleSystem::draw(sf::RenderTarget& target, sf::BlendMode blendMode)
{
    vertices.clear();
    for (const Particle& particle : particles)
    {
        // Fade out over the particle's life.
        sf::Color color = particle.color;
        color.a = static_cast<std::uint8_t>(color.a * (particle.secondsLeft / particle.lifetimeSeconds));

        sf::Vector2f halfSize(particle.size / 2.f, particle.size / 2.f);
        appendQuad(vertices, particle.position - halfSize, particle.position + halfSize, color);
    }
    target.draw(vertices, blendMode);
}

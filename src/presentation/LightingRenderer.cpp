#include "presentation/LightingRenderer.hpp"

#include "presentation/Palette.hpp"
#include "presentation/VertexQuads.hpp"
#include "simulation/SimulatorErrors.hpp"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

namespace
{
    constexpr unsigned int GradientSize = 256; // Pixels per side of the radial gradient texture.

    // Brightness from 1 at the light to 0 at the edge of its radius, with soft shoulders (a smoothstep).
    float lightFalloff(float distanceInRadii)
    {
        if (distanceInRadii >= 1.f)
        {
            return 0.f;
        }
        float smoothed = distanceInRadii * distanceInRadii * (3.f - 2.f * distanceInRadii);
        return 1.f - smoothed;
    }

    sf::Color scaleBrightness(sf::Color color, float brightness)
    {
        std::uint8_t level = static_cast<std::uint8_t>(brightness * 255.f);
        return color * sf::Color(level, level, level);
    }

    // Maps a world position onto the gradient texture, centered on the light, so brightness falls with distance.
    // Positions beyond the light radius land outside the texture, where its black edge repeats.
    sf::Vertex makeLightVertex(sf::Vector2f worldPosition, sf::Vector2f lightOrigin)
    {
        float halfSize = static_cast<float>(GradientSize) / 2.f;
        sf::Vector2f offsetInRadii = (worldPosition - lightOrigin) / LightingRenderer::LightRadius;
        sf::Vector2f textureCoordinates = sf::Vector2f(halfSize, halfSize) + offsetInRadii * halfSize;
        return sf::Vertex{worldPosition, Palette::Light, textureCoordinates};
    }
}

LightingRenderer::LightingRenderer(sf::Vector2u windowSize)
    : visibilityFan(sf::PrimitiveType::TriangleFan)
    , wallLightVertices(sf::PrimitiveType::Triangles)
{
    if (!lightMap.resize(windowSize))
    {
        throw ResourceError("Could not create the " + std::to_string(windowSize.x) + " x "
                            + std::to_string(windowSize.y) + " light map render texture.");
    }

    // Build the radial gradient once, pixel by pixel: white at the center, black at the edge.
    sf::Image gradientImage(sf::Vector2u(GradientSize, GradientSize), sf::Color::Black);
    float center = static_cast<float>(GradientSize - 1) / 2.f;
    for (unsigned int pixelY = 0; pixelY < GradientSize; ++pixelY)
    {
        for (unsigned int pixelX = 0; pixelX < GradientSize; ++pixelX)
        {
            float offsetX = (static_cast<float>(pixelX) - center) / center;
            float offsetY = (static_cast<float>(pixelY) - center) / center;
            float distanceInRadii = std::sqrt(offsetX * offsetX + offsetY * offsetY);
            gradientImage.setPixel(sf::Vector2u(pixelX, pixelY), scaleBrightness(sf::Color::White, lightFalloff(distanceInRadii)));
        }
    }
    if (!radialGradient.loadFromImage(gradientImage))
    {
        throw ResourceError("Could not upload the light gradient texture.");
    }
    radialGradient.setSmooth(true);
}

void LightingRenderer::drawLightMap(sf::RenderTarget& target,
                                    const sf::View& worldView,
                                    const Grid& grid,
                                    sf::Vector2f lightOrigin,
                                    const std::vector<RayHit>& visibility)
{
    // The visibility polygon as a triangle fan: the light's origin, every hit in angle order,
    // then the first hit again to close the loop.
    visibilityFan.clear();
    visibilityFan.append(makeLightVertex(lightOrigin, lightOrigin));
    for (const RayHit& hit : visibility)
    {
        visibilityFan.append(makeLightVertex(hit.point, lightOrigin));
    }
    if (!visibility.empty())
    {
        visibilityFan.append(makeLightVertex(visibility.front().point, lightOrigin));
    }

    // Walls stop the rays, so they sit just outside the polygon. Each wall cell a ray reached
    // gets lit as brightly as the floor at that ray's end, using the nearest hit on that cell.
    const float notHit = std::numeric_limits<float>::infinity();
    nearestHitDistances.assign(grid.getCellCount(), notHit);
    for (const RayHit& hit : visibility)
    {
        if (!grid.contains(hit.cell))
        {
            continue;
        }
        std::size_t cellIndex = grid.indexOf(hit.cell);
        if (hit.distance < nearestHitDistances[cellIndex])
        {
            nearestHitDistances[cellIndex] = hit.distance;
        }
    }

    wallLightVertices.clear();
    for (std::size_t cellIndex = 0; cellIndex < nearestHitDistances.size(); ++cellIndex)
    {
        float distance = nearestHitDistances[cellIndex];
        if (distance >= LightRadius)
        {
            continue;
        }
        GridCell cell = grid.cellAt(cellIndex);
        sf::Vector2f topLeft(static_cast<float>(cell.column), static_cast<float>(cell.row));
        sf::Vector2f bottomRight = topLeft + sf::Vector2f(1.f, 1.f);
        appendQuad(wallLightVertices, topLeft, bottomRight, scaleBrightness(Palette::Light, lightFalloff(distance / LightRadius)));
    }

    // Light map = ambient everywhere + the gradient inside the polygon + the lit walls.
    lightMap.setView(worldView);
    lightMap.clear(Palette::AmbientLight);
    sf::RenderStates polygonStates;
    polygonStates.blendMode = sf::BlendAdd;
    polygonStates.texture = &radialGradient;
    lightMap.draw(visibilityFan, polygonStates);
    lightMap.draw(wallLightVertices, sf::BlendAdd);
    lightMap.display();

    // Multiply: lit pixels keep their color, unlit pixels drop to the ambient level.
    sf::Sprite lightSprite(lightMap.getTexture());
    sf::View previousView = target.getView();
    target.setView(target.getDefaultView());
    target.draw(lightSprite, sf::BlendMultiply);
    target.setView(previousView);
}

const sf::Texture& LightingRenderer::getGlowTexture() const
{
    return radialGradient;
}

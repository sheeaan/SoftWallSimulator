#include "presentation/AlgorithmOverlay.hpp"

#include "presentation/Palette.hpp"
#include "presentation/VertexQuads.hpp"

#include <cmath>
#include <cstdint>

namespace
{
    constexpr float ExploredCellInset = 0.12f; // Tiles. Keeps neighboring explored cells visually separate.
    constexpr float PathDotSize = 0.2f;        // Tiles.
    constexpr float PathFlashSeconds = 0.35f;
    constexpr float PulseSpeed = 8.f;          // Radians per second.
    constexpr float PulseSpacing = 0.6f;       // Radians between neighboring waypoints.

    sf::Color blendColors(sf::Color from, sf::Color to, float amount)
    {
        float keep = 1.f - amount;
        return sf::Color(static_cast<std::uint8_t>(from.r * keep + to.r * amount),
                         static_cast<std::uint8_t>(from.g * keep + to.g * amount),
                         static_cast<std::uint8_t>(from.b * keep + to.b * amount),
                         static_cast<std::uint8_t>(from.a * keep + to.a * amount));
    }
}

AlgorithmOverlay::AlgorithmOverlay()
    : exploredVertices(sf::PrimitiveType::Triangles)
    , rayVertices(sf::PrimitiveType::Lines)
    , outlineVertices(sf::PrimitiveType::LineStrip)
    , pathLineVertices(sf::PrimitiveType::LineStrip)
    , pathDotVertices(sf::PrimitiveType::Triangles)
{
}

void AlgorithmOverlay::toggle()
{
    visible = !visible;
}

bool AlgorithmOverlay::isVisible() const
{
    return visible;
}

void AlgorithmOverlay::flashPath()
{
    flashSecondsLeft = PathFlashSeconds;
}

void AlgorithmOverlay::update(float seconds)
{
    elapsedSeconds += seconds;
    if (flashSecondsLeft > 0.f)
    {
        flashSecondsLeft -= seconds;
    }
}

void AlgorithmOverlay::draw(sf::RenderTarget& target, const World& world)
{
    if (!visible)
    {
        return;
    }

    buildExploredCells(world);
    buildRays(world);
    buildPath(world);

    target.draw(exploredVertices);
    target.draw(rayVertices);
    target.draw(outlineVertices);
    target.draw(pathLineVertices);
    target.draw(pathDotVertices);
}

void AlgorithmOverlay::buildExploredCells(const World& world)
{
    // Color each expanded cell by when A* reached it: early cells violet, late cells teal.
    // The gradient shows the search front sweeping from the enemy toward the player.
    const std::vector<std::size_t>& expandedCells = world.getPathfinder().getExpandedCells();
    exploredVertices.clear();
    for (std::size_t order = 0; order < expandedCells.size(); ++order)
    {
        float progress = static_cast<float>(order) / static_cast<float>(expandedCells.size());
        sf::Color color = blendColors(Palette::ExploredEarly, Palette::ExploredLate, progress);

        GridCell cell = world.getGrid().cellAt(expandedCells[order]);
        float left = static_cast<float>(cell.column);
        float top = static_cast<float>(cell.row);
        appendQuad(exploredVertices,
                   sf::Vector2f(left + ExploredCellInset, top + ExploredCellInset),
                   sf::Vector2f(left + 1.f - ExploredCellInset, top + 1.f - ExploredCellInset),
                   color);
    }
}

void AlgorithmOverlay::buildRays(const World& world)
{
    // One line per ray, from the player to where DDA stopped it, plus the outline of the polygon.
    sf::Vector2f origin = world.getPlayer().getPosition();
    const std::vector<RayHit>& visibility = world.getVisibility();
    rayVertices.clear();
    outlineVertices.clear();
    for (const RayHit& hit : visibility)
    {
        rayVertices.append(sf::Vertex{origin, Palette::Ray});
        rayVertices.append(sf::Vertex{hit.point, Palette::Ray});
        outlineVertices.append(sf::Vertex{hit.point, Palette::VisibilityOutline});
    }
    if (!visibility.empty())
    {
        outlineVertices.append(sf::Vertex{visibility.front().point, Palette::VisibilityOutline});
    }
}

void AlgorithmOverlay::buildPath(const World& world)
{
    // The rest of the path: a line from the enemy through each waypoint still ahead, and a dot on each.
    const Enemy& enemy = world.getEnemy();
    const std::vector<GridCell>& path = enemy.getPath();

    sf::Color pathColor = Palette::Path;
    float dotSize = PathDotSize;
    if (flashSecondsLeft > 0.f)
    {
        pathColor = Palette::PathFlash;
        dotSize = PathDotSize * 1.8f;
    }

    pathLineVertices.clear();
    pathDotVertices.clear();
    pathLineVertices.append(sf::Vertex{enemy.getPosition(), pathColor});
    for (std::size_t waypointIndex = enemy.getNextWaypointIndex(); waypointIndex < path.size(); ++waypointIndex)
    {
        sf::Vector2f center = cellCenter(path[waypointIndex]);
        pathLineVertices.append(sf::Vertex{center, pathColor});

        // A bright pulse runs along the dots from the enemy toward the player.
        float pulse = 0.5f + 0.5f * std::sin(elapsedSeconds * PulseSpeed - static_cast<float>(waypointIndex) * PulseSpacing);
        sf::Color dotColor = pathColor;
        dotColor.a = static_cast<std::uint8_t>(110.f + 145.f * pulse);

        sf::Vector2f halfDot(dotSize / 2.f, dotSize / 2.f);
        appendQuad(pathDotVertices, center - halfDot, center + halfDot, dotColor);
    }
}

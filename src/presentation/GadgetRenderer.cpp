#include "presentation/GadgetRenderer.hpp"

#include "presentation/Fonts.hpp"
#include "presentation/Palette.hpp"
#include "presentation/VertexQuads.hpp"

#include <SFML/Graphics/RenderStates.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace
{
    constexpr float BreachPadSize = 0.38f;   // Tiles.
    constexpr float BreachLightSize = 0.12f;
    constexpr float HardBreachWidth = 0.56f; // Tiles. The hard breach charge is the bigger, brighter unit.
    constexpr float HardBreachHeight = 0.3f;
    constexpr float FuseBarHeight = 0.07f;
    constexpr float DroneRadius = 0.18f;
    constexpr float SpottedMarkerRadius = 0.62f;

    float fuseSecondsFor(ChargeType type)
    {
        if (type == ChargeType::BREACH)
        {
            return World::BreachFuseSeconds;
        }
        return World::HardBreachFuseSeconds;
    }
}

GadgetRenderer::GadgetRenderer(sf::Vector2u windowSize, const Grid& grid, const std::vector<GridCell>& bombSite)
    : font(loadFont("BarlowCondensed-SemiBold.ttf"))
    , siteLabel(font, "BOMB A", static_cast<unsigned int>(static_cast<float>(windowSize.y) * 0.024f))
    , hasBombSite(!bombSite.empty())
    , chargeShapes(sf::PrimitiveType::Triangles)
    , droneBody(DroneRadius, 24)
    , droneSignal(DroneRadius, 24)
    , spottedMarker(SpottedMarkerRadius, 4)
{
    // The label is laid out in pixels, then scaled down to world units, so it sits on the map.
    float pixelsPerTile = static_cast<float>(windowSize.x) / static_cast<float>(grid.getColumnCount());
    siteLabel.setScale(sf::Vector2f(1.f / pixelsPerTile, 1.f / pixelsPerTile));
    siteLabel.setFillColor(Palette::SiteOutline);
    siteLabel.setLetterSpacing(1.4f);

    // Center the label just above the bomb site.
    if (hasBombSite)
    {
        int leftColumn = bombSite.front().column;
        int rightColumn = bombSite.front().column;
        int topRow = bombSite.front().row;
        for (const GridCell& cell : bombSite)
        {
            leftColumn = std::min(leftColumn, cell.column);
            rightColumn = std::max(rightColumn, cell.column);
            topRow = std::min(topRow, cell.row);
        }
        sf::FloatRect bounds = siteLabel.getLocalBounds();
        siteLabel.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y));
        float centerX = (static_cast<float>(leftColumn) + static_cast<float>(rightColumn) + 1.f) / 2.f;
        siteLabel.setPosition(sf::Vector2f(centerX, static_cast<float>(topRow) - 0.15f));
    }

    droneBody.setOrigin(sf::Vector2f(DroneRadius, DroneRadius));
    droneBody.setFillColor(Palette::DroneBody);
    droneBody.setOutlineColor(Palette::DroneRing);
    droneBody.setOutlineThickness(0.05f);
    droneSignal.setOrigin(sf::Vector2f(DroneRadius, DroneRadius));
    droneSignal.setFillColor(sf::Color::Transparent);
    droneSignal.setOutlineThickness(0.03f);

    spottedMarker.setOrigin(sf::Vector2f(SpottedMarkerRadius, SpottedMarkerRadius));
    spottedMarker.setFillColor(sf::Color::Transparent);
    spottedMarker.setOutlineThickness(0.07f);
}

void GadgetRenderer::draw(sf::RenderTarget& target, const World& world, float elapsedSeconds)
{
    if (hasBombSite)
    {
        target.draw(siteLabel);
    }

    chargeShapes.clear();
    for (const Charge& charge : world.getCharges())
    {
        appendCharge(charge, elapsedSeconds);
    }
    target.draw(chargeShapes);

    // The drone, with a ring that pulses outward while the attacker drives it.
    if (world.getDrone().has_value())
    {
        sf::Vector2f dronePosition = world.getDrone().value().getPosition();
        droneBody.setPosition(dronePosition);
        target.draw(droneBody);
        if (world.isDrivingDrone())
        {
            float pulse = elapsedSeconds * 1.5f - std::floor(elapsedSeconds * 1.5f); // 0 to 1, repeating.
            sf::Color signalColor = Palette::DroneRing;
            signalColor.a = static_cast<std::uint8_t>(220.f * (1.f - pulse));
            droneSignal.setOutlineColor(signalColor);
            droneSignal.setScale(sf::Vector2f(1.f + pulse * 2.5f, 1.f + pulse * 2.5f));
            droneSignal.setPosition(dronePosition);
            target.draw(droneSignal);
        }
    }

    // A spotted defender gets an orange diamond that shows through walls, the way Siege marks a ping.
    if (world.isEnemySpotted())
    {
        float pulse = 0.75f + 0.25f * std::sin(elapsedSeconds * 6.f);
        sf::Color markerColor = Palette::SpottedMarker;
        markerColor.a = static_cast<std::uint8_t>(255.f * pulse);
        spottedMarker.setOutlineColor(markerColor);
        spottedMarker.setPosition(world.getEnemy().getPosition());
        target.draw(spottedMarker);
    }
}

void GadgetRenderer::appendCharge(const Charge& charge, float elapsedSeconds)
{
    sf::Vector2f center = cellCenter(charge.wall);
    float fuseLeft = std::max(0.f, charge.secondsLeft / fuseSecondsFor(charge.type)); // 1 when placed, 0 at detonation.

    sf::Vector2f halfSize(BreachPadSize / 2.f, BreachPadSize / 2.f);
    if (charge.type == ChargeType::HARD_BREACH)
    {
        halfSize = sf::Vector2f(HardBreachWidth / 2.f, HardBreachHeight / 2.f);
    }
    appendQuad(chargeShapes, center - halfSize, center + halfSize, Palette::ChargeBody);

    if (charge.type == ChargeType::BREACH)
    {
        // A red light that blinks faster as the fuse burns down.
        float blinksPerSecond = 2.f + 8.f * (1.f - fuseLeft);
        float blinkPhase = elapsedSeconds * blinksPerSecond - std::floor(elapsedSeconds * blinksPerSecond);
        if (blinkPhase < 0.5f)
        {
            sf::Vector2f halfLight(BreachLightSize / 2.f, BreachLightSize / 2.f);
            appendQuad(chargeShapes, center - halfLight, center + halfLight, Palette::BreachChargeLight);
        }
    }
    else
    {
        // The hard breach charge burns: its face glows brighter as the fuse runs out.
        sf::Color glow = Palette::HardBreachGlow;
        glow.a = static_cast<std::uint8_t>(120.f + 135.f * (1.f - fuseLeft));
        sf::Vector2f inset(0.05f, 0.05f);
        appendQuad(chargeShapes, center - halfSize + inset, center + halfSize - inset, glow);
    }

    // The fuse, as a bar under the charge that empties toward detonation.
    float barTop = center.y + halfSize.y + 0.05f;
    float barLeft = center.x - halfSize.x;
    float barWidth = halfSize.x * 2.f;
    appendQuad(chargeShapes, sf::Vector2f(barLeft, barTop), sf::Vector2f(barLeft + barWidth, barTop + FuseBarHeight),
               Palette::ChargeBody);
    appendQuad(chargeShapes, sf::Vector2f(barLeft, barTop), sf::Vector2f(barLeft + barWidth * fuseLeft, barTop + FuseBarHeight),
               Palette::HudMessage);
}

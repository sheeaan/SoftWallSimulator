#include "presentation/Game.hpp"

#include "presentation/Palette.hpp"
#include "simulation/Level.hpp"

#include <SFML/System/Angle.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

namespace
{
    constexpr float MaximumFrameSeconds = 0.05f; // Longer frames (a dragged window, a breakpoint) are clamped.

    constexpr float PlayerGlowDiameter = 3.5f;  // Tiles.
    constexpr float EnemyDrawRadius = 0.38f;    // Tiles, to the diamond's corners.
    constexpr float EnemyGlowDiameter = 3.f;    // Tiles.
    constexpr float EnemyFadePerSecond = 6.f;   // A full fade takes a sixth of a second.
    constexpr float EnemyPulseSpeed = 5.f;      // Radians per second.
    constexpr float EnemySpinDegreesPerSecond = 90.f;

    constexpr float MaximumShakeOffset = 0.35f; // Tiles, at full trauma.
    constexpr float ShakeDecayPerSecond = 1.6f;
    constexpr float DestroyShake = 0.55f;
    constexpr float DamageShake = 0.22f;
    constexpr float HardWallShake = 0.08f;

    // About 80% of the desktop at 16:9, so the window looks the same size on any display.
    sf::Vector2u chooseWindowSize()
    {
        sf::Vector2u desktopSize = sf::VideoMode::getDesktopMode().size;
        unsigned int width = desktopSize.x * 4 / 5;
        unsigned int height = width * 9 / 16;
        unsigned int maximumHeight = desktopSize.y * 4 / 5;
        if (height > maximumHeight)
        {
            height = maximumHeight;
            width = height * 16 / 9;
        }
        return sf::Vector2u(width, height);
    }
}

Game::Game()
    : window(sf::VideoMode(chooseWindowSize()), "Soft Wall Simulator", sf::Style::Titlebar | sf::Style::Close)
    , world(parseLevel(getDefaultLevelRows()))
    , tileMapRenderer(world.getGrid())
    , lightingRenderer(window.getSize())
    , statisticsPanel(window.getSize())
    , debrisParticles(600)
    , sparkParticles(800)
    , playerShape(Player::HalfSize, 40)
    , playerGlow(lightingRenderer.getGlowTexture())
    , enemyShape(EnemyDrawRadius, 4) // Four points make a diamond, which reads as hostile next to the round player.
    , enemyGlow(lightingRenderer.getGlowTexture())
{
    window.setVerticalSyncEnabled(true);

    // One world unit is one tile, so simulation positions draw without any conversion.
    sf::Vector2f mapSize(static_cast<float>(world.getGrid().getColumnCount()),
                         static_cast<float>(world.getGrid().getRowCount()));
    worldView = sf::View(sf::FloatRect(sf::Vector2f(0.f, 0.f), mapSize));
    cameraView = worldView;

    playerShape.setOrigin(sf::Vector2f(Player::HalfSize, Player::HalfSize));
    playerShape.setFillColor(Palette::PlayerBody);
    enemyShape.setOrigin(sf::Vector2f(EnemyDrawRadius, EnemyDrawRadius));

    sf::Vector2f glowTextureSize(lightingRenderer.getGlowTexture().getSize());
    playerGlow.setOrigin(glowTextureSize / 2.f);
    playerGlow.setScale(sf::Vector2f(PlayerGlowDiameter / glowTextureSize.x, PlayerGlowDiameter / glowTextureSize.y));
    playerGlow.setColor(Palette::PlayerGlow);
    enemyGlow.setOrigin(glowTextureSize / 2.f);
    enemyGlow.setScale(sf::Vector2f(EnemyGlowDiameter / glowTextureSize.x, EnemyGlowDiameter / glowTextureSize.y));
}

void Game::run()
{
    sf::Clock frameClock;
    while (window.isOpen())
    {
        float seconds = frameClock.restart().asSeconds();
        if (seconds > MaximumFrameSeconds)
        {
            seconds = MaximumFrameSeconds;
        }

        processEvents();
        if (!window.isOpen())
        {
            break;
        }
        update(seconds);
        render();
    }
}

void Game::processEvents()
{
    while (const std::optional<sf::Event> event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window.close();
        }
        else if (const sf::Event::KeyPressed* keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            if (keyPressed->scancode == sf::Keyboard::Scan::Tab)
            {
                algorithmOverlay.toggle();
            }
        }
        else if (const sf::Event::MouseButtonPressed* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
        {
            if (mousePressed->button == sf::Mouse::Button::Left)
            {
                handleLeftClick(mousePressed->position);
            }
        }
    }
}

void Game::handleLeftClick(sf::Vector2i pixelPosition)
{
    // Pixel to world through the same view the last frame was drawn with, so the hit lands on the tile
    // under the cursor even mid-shake. World to cell happens inside World::damageTileAt.
    sf::Vector2f worldPosition = window.mapPixelToCoords(pixelPosition, cameraView);
    GridCell clickedCell = cellContaining(worldPosition);
    sf::Vector2f clickedCenter = cellCenter(clickedCell);

    bool clickedHardWall = false;
    if (world.getGrid().contains(clickedCell))
    {
        clickedHardWall = world.getGrid().getTile(clickedCell).type == TileType::HARD_WALL;
    }

    DamageResult result = world.damageTileAt(worldPosition);
    if (result == DamageResult::DESTROYED)
    {
        tileMapRenderer.rebuildWalls(world.getGrid());
        tileMapRenderer.flashTile(clickedCell);
        debrisParticles.spawnBurst(clickedCenter, 32, Palette::Debris, 5.f, 0.9f, 0.2f);
        sparkParticles.spawnBurst(clickedCenter, 40, Palette::Spark, 10.f, 0.5f, 0.1f);
        algorithmOverlay.flashPath();
        shakeTrauma = std::min(1.f, shakeTrauma + DestroyShake);
    }
    else if (result == DamageResult::DAMAGED)
    {
        tileMapRenderer.rebuildWalls(world.getGrid());
        tileMapRenderer.flashTile(clickedCell);
        debrisParticles.spawnBurst(clickedCenter, 10, Palette::Debris, 3.5f, 0.6f, 0.15f);
        sparkParticles.spawnBurst(clickedCenter, 14, Palette::Spark, 8.f, 0.35f, 0.09f);
        shakeTrauma = std::min(1.f, shakeTrauma + DamageShake);
    }
    else if (clickedHardWall)
    {
        // Hard walls shrug the hit off: a flash and a few cold sparks, but no damage.
        tileMapRenderer.flashTile(clickedCell);
        sparkParticles.spawnBurst(clickedCenter, 12, Palette::HardWallSpark, 7.f, 0.3f, 0.08f);
        shakeTrauma = std::min(1.f, shakeTrauma + HardWallShake);
    }
}

void Game::update(float seconds)
{
    elapsedSeconds += seconds;
    world.update(seconds, readMovementInput());
    algorithmOverlay.update(seconds);
    statisticsPanel.update(seconds, world);
    tileMapRenderer.update(seconds);
    debrisParticles.update(seconds);
    sparkParticles.update(seconds);

    // Fade the enemy in or out instead of popping at the edge of a shadow.
    // The overlay shows it through walls.
    float targetVisibility = 0.f;
    if (world.canPlayerSeeEnemy() || algorithmOverlay.isVisible())
    {
        targetVisibility = 1.f;
    }
    float fadeStep = EnemyFadePerSecond * seconds;
    if (enemyVisibility < targetVisibility)
    {
        enemyVisibility = std::min(targetVisibility, enemyVisibility + fadeStep);
    }
    else
    {
        enemyVisibility = std::max(targetVisibility, enemyVisibility - fadeStep);
    }

    // Screen shake: trauma decays over time, and the offset grows with its square,
    // so small hits barely nudge the camera and a breaking wall kicks it hard.
    shakeTrauma = std::max(0.f, shakeTrauma - ShakeDecayPerSecond * seconds);
    float shakeOffset = MaximumShakeOffset * shakeTrauma * shakeTrauma;
    std::uniform_real_distribution<float> shakeDirection(-1.f, 1.f);
    cameraView = worldView;
    cameraView.move(sf::Vector2f(shakeDirection(shakeRandom), shakeDirection(shakeRandom)) * shakeOffset);
}

void Game::render()
{
    window.clear(Palette::Background);
    window.setView(cameraView);

    // Everything drawn before the light map is darkened where the rays cannot reach.
    tileMapRenderer.drawFloor(window);
    debrisParticles.draw(window, sf::BlendAlpha);
    tileMapRenderer.drawWalls(window);
    lightingRenderer.drawLightMap(window, cameraView, world.getGrid(), world.getPlayer().getPosition(),
                                  world.getVisibility());

    // Everything drawn after it stays visible in the dark.
    tileMapRenderer.drawWallEdges(window);
    tileMapRenderer.drawFlashes(window);
    algorithmOverlay.draw(window, world);

    // The enemy's glow pulses, and both glow and body fade with line of sight.
    float pulse = 0.75f + 0.25f * std::sin(elapsedSeconds * EnemyPulseSpeed);
    sf::Color enemyGlowColor = Palette::EnemyGlow;
    enemyGlowColor.a = static_cast<std::uint8_t>(enemyGlowColor.a * pulse * enemyVisibility);
    enemyGlow.setColor(enemyGlowColor);
    enemyGlow.setPosition(world.getEnemy().getPosition());
    window.draw(enemyGlow, sf::BlendAdd);

    sf::Color enemyBodyColor = Palette::EnemyBody;
    enemyBodyColor.a = static_cast<std::uint8_t>(255.f * enemyVisibility);
    enemyShape.setFillColor(enemyBodyColor);
    enemyShape.setPosition(world.getEnemy().getPosition());
    enemyShape.setRotation(sf::degrees(elapsedSeconds * EnemySpinDegreesPerSecond));
    window.draw(enemyShape);

    playerGlow.setPosition(world.getPlayer().getPosition());
    window.draw(playerGlow, sf::BlendAdd);
    playerShape.setPosition(world.getPlayer().getPosition());
    window.draw(playerShape);

    sparkParticles.draw(window, sf::BlendAdd);
    statisticsPanel.draw(window);

    window.display();
}

sf::Vector2f Game::readMovementInput() const
{
    sf::Vector2f direction(0.f, 0.f);
    if (!window.hasFocus())
    {
        return direction;
    }

    // Scancodes are physical key positions, so WASD works on any keyboard layout.
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::W))
    {
        direction.y -= 1.f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::S))
    {
        direction.y += 1.f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A))
    {
        direction.x -= 1.f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D))
    {
        direction.x += 1.f;
    }
    return direction;
}

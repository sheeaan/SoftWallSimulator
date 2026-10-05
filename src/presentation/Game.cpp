#include "presentation/Game.hpp"

#include "presentation/Palette.hpp"

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

    constexpr float PlayerLightRadius = 24.f;   // Tiles.
    constexpr float DroneLightRadius = 11.f;    // Tiles. The drone's camera sees less far.
    constexpr float PlayerGlowDiameter = 3.5f;  // Tiles.
    constexpr float EnemyDrawRadius = 0.38f;    // Tiles, to the diamond's corners.
    constexpr float EnemyGlowDiameter = 3.f;    // Tiles.
    constexpr float EnemyFadePerSecond = 6.f;   // A full fade takes a sixth of a second.
    constexpr float EnemyPulseSpeed = 5.f;      // Radians per second.
    constexpr float EnemySpinDegreesPerSecond = 90.f;

    constexpr float MaximumShakeOffset = 0.35f; // Tiles, at full trauma.
    constexpr float ShakeDecayPerSecond = 1.6f;
    constexpr float BreachShake = 0.9f;
    constexpr float HardBreachShake = 0.75f;
    constexpr float DestroyShake = 0.55f;
    constexpr float DamageShake = 0.22f;
    constexpr float RicochetShake = 0.08f;
    constexpr std::size_t ParticleCapacity = 1200;

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
    , level(parseLevel(getDefaultLevelRows()))
    , world(level)
    , tileMapRenderer(world.getGrid(), world.getBombSite())
    , drawnGridRevision(world.getGrid().getRevision())
    , lightingRenderer(window.getSize())
    , droneCameraFilter(window.getSize())
    , statisticsPanel(window.getSize())
    , headsUpDisplay(window.getSize())
    , mathView(window.getSize(), world.getGrid())
    , gadgetRenderer(window.getSize(), world.getGrid(), world.getBombSite())
    , debrisParticles(ParticleCapacity)
    , sparkParticles(ParticleCapacity)
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
            else if (keyPressed->scancode == sf::Keyboard::Scan::M)
            {
                mathView.toggle();
            }
            else if (keyPressed->scancode == sf::Keyboard::Scan::Q)
            {
                handleChargeKey(ChargeType::BREACH);
            }
            else if (keyPressed->scancode == sf::Keyboard::Scan::E)
            {
                handleChargeKey(ChargeType::HARD_BREACH);
            }
            else if (keyPressed->scancode == sf::Keyboard::Scan::F)
            {
                world.toggleDrone();
            }
            else if (keyPressed->scancode == sf::Keyboard::Scan::R)
            {
                restartRound();
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
    if (!world.getGrid().contains(clickedCell))
    {
        return;
    }

    if (world.getRound().getPhase() == RoundPhase::PREPARATION)
    {
        headsUpDisplay.showMessage("WAIT FOR THE ACTION PHASE");
        return;
    }
    if (world.isDrivingDrone())
    {
        headsUpDisplay.showMessage("THE DRONE CANNOT SHOOT.  PRESS F TO SWITCH BACK.");
        return;
    }

    TileType typeBeforeShot = world.getGrid().getTile(clickedCell).type;
    DamageResult result = world.damageTileAt(worldPosition);
    if (result == DamageResult::DESTROYED && typeBeforeShot == TileType::BARRICADE)
    {
        // The planks give way in one hit.
        tileMapRenderer.flashTile(clickedCell);
        debrisParticles.spawnBurst(clickedCenter, 30, Palette::Splinter, 7.f, 0.7f, 0.12f);
        algorithmOverlay.flashPath();
        shakeTrauma = std::min(1.f, shakeTrauma + DamageShake);
    }
    else if (result == DamageResult::DESTROYED)
    {
        // The whole board comes down: a cloud of plaster dust and a spray of splintered studs.
        tileMapRenderer.flashTile(clickedCell);
        debrisParticles.spawnBurst(clickedCenter, 40, Palette::PlasterDust, 3.5f, 1.2f, 0.26f);
        debrisParticles.spawnBurst(clickedCenter, 30, Palette::Splinter, 8.f, 0.7f, 0.1f);
        algorithmOverlay.flashPath();
        shakeTrauma = std::min(1.f, shakeTrauma + DestroyShake);
    }
    else if (result == DamageResult::DAMAGED)
    {
        // A bullet punches through drywall: a puff of dust and a few splinters where it hit.
        tileMapRenderer.flashTile(clickedCell);
        debrisParticles.spawnBurst(worldPosition, 14, Palette::PlasterDust, 2.5f, 0.8f, 0.16f);
        debrisParticles.spawnBurst(worldPosition, 10, Palette::Splinter, 6.f, 0.5f, 0.08f);
        shakeTrauma = std::min(1.f, shakeTrauma + DamageShake);
    }
    else if (typeBeforeShot == TileType::REINFORCED_WALL)
    {
        // Steel takes no damage. The bullet sparks off it.
        tileMapRenderer.flashTile(clickedCell);
        sparkParticles.spawnBurst(worldPosition, 16, Palette::Spark, 9.f, 0.3f, 0.08f);
        shakeTrauma = std::min(1.f, shakeTrauma + RicochetShake);
    }
    else if (typeBeforeShot == TileType::HARD_WALL)
    {
        // Concrete takes no damage either. The bullet chips off a little dust.
        debrisParticles.spawnBurst(worldPosition, 8, Palette::ConcreteSpeckle, 3.f, 0.4f, 0.1f);
        shakeTrauma = std::min(1.f, shakeTrauma + RicochetShake);
    }
}

void Game::handleChargeKey(ChargeType type)
{
    ChargePlacement placement = world.placeCharge(type, getCursorWorldPosition());
    switch (placement)
    {
    case ChargePlacement::PLACED:
        break;
    case ChargePlacement::NONE_LEFT:
        headsUpDisplay.showMessage("NONE LEFT");
        break;
    case ChargePlacement::TOO_FAR:
        headsUpDisplay.showMessage("STAND NEXT TO THE WALL, THEN AIM AT IT");
        break;
    case ChargePlacement::WRONG_SURFACE:
        if (type == ChargeType::BREACH)
        {
            headsUpDisplay.showMessage("BREACH CHARGES OPEN DRYWALL AND BARRICADES, NOT STEEL");
        }
        else
        {
            headsUpDisplay.showMessage("HARD BREACH CHARGES GO ON DRYWALL OR STEEL");
        }
        break;
    case ChargePlacement::ALREADY_CHARGED:
        headsUpDisplay.showMessage("THAT WALL ALREADY HAS A CHARGE");
        break;
    case ChargePlacement::NOT_NOW:
        headsUpDisplay.showMessage("NO BREACHING RIGHT NOW");
        break;
    }
}

void Game::restartRound()
{
    // A new round is a new world built from the same level; everything that drew the old one starts over.
    world = World(level);
    tileMapRenderer = TileMapRenderer(world.getGrid(), world.getBombSite());
    drawnGridRevision = world.getGrid().getRevision();
    debrisParticles = ParticleSystem(ParticleCapacity);
    sparkParticles = ParticleSystem(ParticleCapacity);
    mathView.invalidate();
    enemyVisibility = 0.f;
    shakeTrauma = 0.f;
}

void Game::update(float seconds)
{
    elapsedSeconds += seconds;
    world.update(seconds, readMovementInput());
    spawnWorldEffects();

    // Any change to the tiles, from a shot, a charge, or a reinforcement, rebuilds the wall vertices once.
    if (world.getGrid().getRevision() != drawnGridRevision)
    {
        tileMapRenderer.rebuildWalls(world.getGrid());
        drawnGridRevision = world.getGrid().getRevision();
    }

    algorithmOverlay.update(seconds);
    statisticsPanel.update(seconds, world);
    headsUpDisplay.update(world, seconds);
    tileMapRenderer.update(seconds);
    debrisParticles.update(seconds);
    sparkParticles.update(seconds);
    mathView.update(world, getCursorWorldPosition());

    // Fade the enemy in or out instead of popping at the edge of a shadow. The attacker or the drone
    // must see it directly; a spotted mark alone shows only the orange outline. The overlay shows it through walls.
    float targetVisibility = 0.f;
    if (world.canPlayerSeeEnemy() || world.canDroneSeeEnemy() || algorithmOverlay.isVisible())
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

void Game::spawnWorldEffects()
{
    for (const Detonation& detonation : world.getDetonations())
    {
        sf::Vector2f center = cellCenter(detonation.wall);
        if (detonation.type == ChargeType::BREACH)
        {
            // A breach charge blows the wall out: a big cloud of dust and splinters, and a hard kick.
            debrisParticles.spawnBurst(center, 60, Palette::PlasterDust, 4.5f, 1.3f, 0.3f);
            debrisParticles.spawnBurst(center, 40, Palette::Splinter, 9.f, 0.8f, 0.12f);
            shakeTrauma = std::min(1.f, shakeTrauma + BreachShake);
        }
        else
        {
            // The hard breach charge finishes burning through: a shower of molten sparks.
            sparkParticles.spawnBurst(center, 80, Palette::MoltenSpark, 7.f, 0.9f, 0.1f);
            debrisParticles.spawnBurst(center, 20, Palette::PlasterDust, 3.f, 1.f, 0.24f);
            shakeTrauma = std::min(1.f, shakeTrauma + HardBreachShake);
        }
        for (const GridCell& cell : detonation.destroyedCells)
        {
            tileMapRenderer.flashTile(cell);
        }
        if (!detonation.destroyedCells.empty())
        {
            algorithmOverlay.flashPath();
        }
    }

    // A wall turning to steel flashes and throws a few cold sparks.
    for (const GridCell& cell : world.getReinforcedCells())
    {
        tileMapRenderer.flashTile(cell);
        sparkParticles.spawnBurst(cellCenter(cell), 14, Palette::ReinforceSpark, 5.f, 0.35f, 0.07f);
    }

    // A burning hard breach charge sprays molten sparks the whole time its fuse runs.
    for (const Charge& charge : world.getCharges())
    {
        if (charge.type == ChargeType::HARD_BREACH)
        {
            sparkParticles.spawnBurst(cellCenter(charge.wall), 2, Palette::MoltenSpark, 3.5f, 0.35f, 0.07f);
        }
    }
}

void Game::render()
{
    // The world draws into the drone camera's scene texture, not straight to the window.
    sf::RenderTexture& scene = droneCameraFilter.getSceneTexture();
    scene.clear(Palette::Background);
    scene.setView(cameraView);

    // Everything drawn before the light map is darkened where the rays cannot reach.
    tileMapRenderer.drawFloor(scene);
    debrisParticles.draw(scene, sf::BlendAlpha);
    tileMapRenderer.drawWalls(scene);

    // The attacker's light, plus the drone's while the attacker drives it.
    lightingRenderer.beginLightMap(cameraView);
    lightingRenderer.addLight(world.getGrid(), world.getPlayer().getPosition(), world.getVisibility(), PlayerLightRadius,
                              Palette::Light);
    if (world.isDrivingDrone() && world.getDrone().has_value())
    {
        lightingRenderer.addLight(world.getGrid(), world.getDrone().value().getPosition(), world.getDroneVisibility(),
                                  DroneLightRadius, Palette::DroneLight);
    }
    lightingRenderer.applyLightMap(scene);

    // Everything drawn after it stays visible in the dark.
    tileMapRenderer.drawWallEdges(scene);
    tileMapRenderer.drawFlashes(scene);
    algorithmOverlay.draw(scene, world);
    gadgetRenderer.draw(scene, world, elapsedSeconds);

    // The enemy's glow pulses, and both glow and body fade with line of sight.
    float pulse = 0.75f + 0.25f * std::sin(elapsedSeconds * EnemyPulseSpeed);
    sf::Color enemyGlowColor = Palette::EnemyGlow;
    enemyGlowColor.a = static_cast<std::uint8_t>(enemyGlowColor.a * pulse * enemyVisibility);
    enemyGlow.setColor(enemyGlowColor);
    enemyGlow.setPosition(world.getEnemy().getPosition());
    scene.draw(enemyGlow, sf::BlendAdd);

    sf::Color enemyBodyColor = Palette::EnemyBody;
    enemyBodyColor.a = static_cast<std::uint8_t>(255.f * enemyVisibility);
    enemyShape.setFillColor(enemyBodyColor);
    enemyShape.setPosition(world.getEnemy().getPosition());
    enemyShape.setRotation(sf::degrees(elapsedSeconds * EnemySpinDegreesPerSecond));
    scene.draw(enemyShape);

    playerGlow.setPosition(world.getPlayer().getPosition());
    scene.draw(playerGlow, sf::BlendAdd);
    playerShape.setPosition(world.getPlayer().getPosition());
    scene.draw(playerShape);

    sparkParticles.draw(scene, sf::BlendAdd);

    // The camera filter copies the world to the window; the math view and the heads-up display go on top, unfiltered.
    window.clear(Palette::Background);
    droneCameraFilter.drawToWindow(window, elapsedSeconds);
    mathView.draw(window, cameraView);
    statisticsPanel.draw(window);
    headsUpDisplay.draw(window);
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

sf::Vector2f Game::getCursorWorldPosition() const
{
    // The mouse, mapped into the world through the camera the last frame was drawn with.
    return window.mapPixelToCoords(sf::Mouse::getPosition(window), cameraView);
}

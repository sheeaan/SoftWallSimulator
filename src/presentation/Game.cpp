#include "presentation/Game.hpp"

#include "presentation/Palette.hpp"
#include "simulation/Level.hpp"

#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <optional>

namespace
{
    constexpr float MaximumFrameSeconds = 0.05f; // Longer frames (a dragged window, a breakpoint) are clamped.
    constexpr float PlayerGlowDiameter = 3.5f;   // Tiles.

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
    , playerShape(Player::HalfSize, 40)
    , playerGlow(lightingRenderer.getGlowTexture())
{
    window.setVerticalSyncEnabled(true);

    // One world unit is one tile, so simulation positions draw without any conversion.
    sf::Vector2f mapSize(static_cast<float>(world.getGrid().getColumnCount()),
                         static_cast<float>(world.getGrid().getRowCount()));
    worldView = sf::View(sf::FloatRect(sf::Vector2f(0.f, 0.f), mapSize));

    playerShape.setOrigin(sf::Vector2f(Player::HalfSize, Player::HalfSize));
    playerShape.setFillColor(Palette::Player);

    sf::Vector2f glowTextureSize(lightingRenderer.getGlowTexture().getSize());
    playerGlow.setOrigin(glowTextureSize / 2.f);
    playerGlow.setScale(sf::Vector2f(PlayerGlowDiameter / glowTextureSize.x, PlayerGlowDiameter / glowTextureSize.y));
    playerGlow.setColor(Palette::PlayerGlow);
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
    }
}

void Game::update(float seconds)
{
    world.update(seconds, readMovementInput());
}

void Game::render()
{
    window.clear(Palette::Background);
    window.setView(worldView);

    // Everything drawn before the light map is darkened where the rays cannot reach.
    tileMapRenderer.drawFloor(window);
    tileMapRenderer.drawWalls(window);
    lightingRenderer.drawLightMap(window, worldView, world.getGrid(), world.getPlayer().getPosition(),
                                  world.getVisibility());

    // Everything drawn after it stays visible in the dark.
    tileMapRenderer.drawWallEdges(window);

    playerGlow.setPosition(world.getPlayer().getPosition());
    window.draw(playerGlow, sf::BlendAdd);
    playerShape.setPosition(world.getPlayer().getPosition());
    window.draw(playerShape);

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

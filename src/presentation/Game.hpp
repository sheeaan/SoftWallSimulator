#pragma once

#include "presentation/LightingRenderer.hpp"
#include "presentation/TileMapRenderer.hpp"
#include "simulation/World.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/View.hpp>

// Owns the window and the frame loop: read input, update the world, draw it.
class Game
{
public:
    Game();

    void run();

private:
    void processEvents();
    void update(float seconds);
    void render();
    sf::Vector2f readMovementInput() const;

    sf::RenderWindow window;
    sf::View worldView;
    World world;
    TileMapRenderer tileMapRenderer;
    LightingRenderer lightingRenderer;
    sf::CircleShape playerShape;
    sf::Sprite playerGlow;
};

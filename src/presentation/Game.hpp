#pragma once

#include "presentation/AlgorithmOverlay.hpp"
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
    AlgorithmOverlay algorithmOverlay;
    sf::CircleShape playerShape;
    sf::Sprite playerGlow;
    sf::CircleShape enemyShape;
    sf::Sprite enemyGlow;
    float enemyVisibility = 0.f; // 0 is hidden, 1 is fully visible.
    float elapsedSeconds = 0.f;
};

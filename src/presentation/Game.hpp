#pragma once

#include "presentation/AlgorithmOverlay.hpp"
#include "presentation/DroneCameraFilter.hpp"
#include "presentation/GadgetRenderer.hpp"
#include "presentation/HeadsUpDisplay.hpp"
#include "presentation/LightingRenderer.hpp"
#include "presentation/MathView.hpp"
#include "presentation/ParticleSystem.hpp"
#include "presentation/StatisticsPanel.hpp"
#include "presentation/TileMapRenderer.hpp"
#include "simulation/Level.hpp"
#include "simulation/World.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/View.hpp>

#include <random>

// Owns the window and the frame loop: read input, update the world, draw it.
class Game
{
public:
    Game();

    void run();

private:
    void processEvents();
    void handleLeftClick(sf::Vector2i pixelPosition);
    void handleChargeKey(ChargeType type);
    void restartRound();
    void update(float seconds);
    void spawnWorldEffects();
    void render();
    sf::Vector2f readMovementInput() const;
    sf::Vector2f getCursorWorldPosition() const;

    sf::RenderWindow window;
    sf::View worldView;  // The whole map, one world unit per tile.
    sf::View cameraView; // worldView plus this frame's screen shake. Drawing and clicks both use it.
    Level level;         // Kept so that R can start the round over.
    World world;
    TileMapRenderer tileMapRenderer;
    unsigned int drawnGridRevision = 0; // The grid revision the wall vertices were built from.
    LightingRenderer lightingRenderer;
    DroneCameraFilter droneCameraFilter;
    AlgorithmOverlay algorithmOverlay;
    StatisticsPanel statisticsPanel;
    HeadsUpDisplay headsUpDisplay;
    MathView mathView;
    GadgetRenderer gadgetRenderer;
    ParticleSystem debrisParticles;
    ParticleSystem sparkParticles;
    sf::CircleShape playerShape;
    sf::Sprite playerGlow;
    sf::CircleShape enemyShape;
    sf::Sprite enemyGlow;
    float enemyVisibility = 0.f; // 0 is hidden, 1 is fully visible.
    float shakeTrauma = 0.f;     // 0 is still, 1 is the strongest shake. Hits add to it; it decays over time.
    float elapsedSeconds = 0.f;
    std::mt19937 shakeRandom;
};

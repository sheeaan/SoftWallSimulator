#pragma once

#include <SFML/Graphics/Color.hpp>

// Every color lives here, so the look can be tuned in one place.
namespace Palette
{
    inline constexpr sf::Color Background{7, 9, 16};

    // Floor and wall bodies are multiplied by the light map, so these are their fully lit colors.
    inline constexpr sf::Color FloorDark{58, 66, 92};
    inline constexpr sf::Color FloorLight{63, 72, 99};
    inline constexpr sf::Color HardWall{30, 35, 52};
    inline constexpr sf::Color SoftWall{170, 100, 44};

    // Edges skip the light map, so the outline of the map stays visible in the dark.
    inline constexpr sf::Color HardWallEdge{66, 82, 120};
    inline constexpr sf::Color SoftWallEdge{186, 118, 52};

    // The light map starts at the ambient level; the player's light adds to it.
    inline constexpr sf::Color AmbientLight{64, 70, 96};
    inline constexpr sf::Color Light{255, 240, 214};

    // Glow alphas set the strength of their additive blend.
    inline constexpr sf::Color PlayerBody{96, 240, 255};
    inline constexpr sf::Color PlayerGlow{60, 200, 255, 150};
    inline constexpr sf::Color EnemyBody{255, 60, 120};
    inline constexpr sf::Color EnemyGlow{255, 40, 110, 170};

    // Algorithm overlay.
    inline constexpr sf::Color ExploredEarly{150, 90, 255, 60};
    inline constexpr sf::Color ExploredLate{40, 220, 200, 60};
    inline constexpr sf::Color Ray{255, 214, 120, 30};
    inline constexpr sf::Color VisibilityOutline{255, 214, 120, 190};
    inline constexpr sf::Color Path{255, 70, 140, 220};
    inline constexpr sf::Color PathFlash{255, 255, 255, 255};
}

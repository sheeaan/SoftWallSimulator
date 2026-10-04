#pragma once

#include <SFML/Graphics/Color.hpp>

// Every color lives here, so the look can be tuned in one place.
namespace Palette
{
    inline constexpr sf::Color Background{7, 9, 16};

    // Floor and wall bodies are multiplied by the light map, so these are their fully lit colors.
    inline constexpr sf::Color FloorDark{58, 66, 92};
    inline constexpr sf::Color FloorLight{63, 72, 99};
    inline constexpr sf::Color HardWall{44, 52, 76};
    inline constexpr sf::Color SoftWall{170, 100, 44};

    // Edges skip the light map, so the outline of the map stays visible in the dark.
    inline constexpr sf::Color HardWallEdge{66, 82, 120};
    inline constexpr sf::Color SoftWallEdge{186, 118, 52};

    // The light map starts at the ambient level; the player's light adds to it.
    inline constexpr sf::Color AmbientLight{64, 70, 96};
    inline constexpr sf::Color Light{255, 240, 214};

    inline constexpr sf::Color Player{96, 240, 255};
    inline constexpr sf::Color PlayerGlow{60, 200, 255, 150}; // Alpha sets the strength of the additive glow.
}

#pragma once

#include <SFML/Graphics/Color.hpp>

// Every color lives here, so the look can be tuned in one place.
namespace Palette
{
    inline constexpr sf::Color Background{7, 9, 16};

    inline constexpr sf::Color FloorDark{46, 54, 78};
    inline constexpr sf::Color FloorLight{50, 59, 85};

    inline constexpr sf::Color HardWall{30, 36, 54};
    inline constexpr sf::Color HardWallEdge{80, 98, 142};

    inline constexpr sf::Color SoftWall{150, 88, 38};
    inline constexpr sf::Color SoftWallEdge{205, 132, 60};

    inline constexpr sf::Color Player{96, 240, 255};
}

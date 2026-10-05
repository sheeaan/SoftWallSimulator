#pragma once

#include "simulation/SimulatorErrors.hpp"

#include <SFML/Graphics/Font.hpp>

#include <string>

// Loads a font from assets/fonts. CMake passes the absolute path of the assets folder.
inline sf::Font loadFont(const std::string& fileName)
{
    std::string path = std::string(SOFT_WALL_ASSET_DIRECTORY) + "/fonts/" + fileName;
    sf::Font font;
    if (!font.openFromFile(path))
    {
        throw ResourceError("Could not load the font " + path + ".");
    }
    return font;
}

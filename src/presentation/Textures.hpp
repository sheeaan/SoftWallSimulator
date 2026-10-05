#pragma once

#include "simulation/SimulatorErrors.hpp"

#include <SFML/Graphics/Texture.hpp>

#include <string>

// Loads an image from assets/siege, the official fan-kit art. CMake passes the absolute path of the assets folder.
inline sf::Texture loadSiegeTexture(const std::string& fileName)
{
    std::string path = std::string(SOFT_WALL_ASSET_DIRECTORY) + "/siege/" + fileName;
    sf::Texture texture;
    if (!texture.loadFromFile(path))
    {
        throw ResourceError("Could not load the image " + path + ".");
    }
    texture.setSmooth(true);
    return texture;
}

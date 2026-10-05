#pragma once

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>

// Makes the world look like a drone's camera feed, as in Siege: film grain, scanlines, and a vignette.
// The world draws into a render texture first; a fragment shader then copies it to the window.
// The heads-up display draws afterwards, straight to the window, so its text stays sharp.
class DroneCameraFilter
{
public:
    explicit DroneCameraFilter(sf::Vector2u windowSize);

    // Draw the world here.
    sf::RenderTexture& getSceneTexture();

    // Copies the finished world to the window through the shader.
    void drawToWindow(sf::RenderTarget& window, float elapsedSeconds);

private:
    sf::RenderTexture sceneTexture;
    sf::Shader shader;
    bool shaderAvailable = false; // Without shader support, the world is copied unfiltered.
};

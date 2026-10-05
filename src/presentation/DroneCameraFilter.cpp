#include "presentation/DroneCameraFilter.hpp"

#include "simulation/SimulatorErrors.hpp"

#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/View.hpp>

#include <string>

namespace
{
    // GLSL 1.10, the version SFML's default pipeline expects.
    const char* const FragmentShaderSource = R"(
uniform sampler2D scene;
uniform float time;
uniform vec2 resolution;

// A cheap hash: a pseudo-random number from 0 to 1 for any 2D point.
float random(vec2 point)
{
    return fract(sin(dot(point, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec2 textureCoordinates = gl_TexCoord[0].xy;
    vec3 color = texture2D(scene, textureCoordinates).rgb;

    // Scanlines: a soft dark band every three pixel rows.
    float pixelRow = textureCoordinates.y * resolution.y;
    color *= 0.9 + 0.1 * sin(pixelRow * 2.0943951);

    // Film grain: noise that differs per pixel and changes every frame.
    float grain = random(textureCoordinates * resolution + vec2(time * 37.0, time * 91.0)) - 0.5;
    color += grain * 0.07;

    // Vignette: darken toward the corners.
    vec2 fromCenter = textureCoordinates - vec2(0.5);
    color *= clamp(1.0 - dot(fromCenter, fromCenter) * 1.2, 0.0, 1.0);

    gl_FragColor = vec4(color, 1.0);
}
)";
}

DroneCameraFilter::DroneCameraFilter(sf::Vector2u windowSize)
{
    if (!sceneTexture.resize(windowSize))
    {
        throw ResourceError("Could not create the " + std::to_string(windowSize.x) + " x "
                            + std::to_string(windowSize.y) + " scene render texture.");
    }

    if (!sf::Shader::isAvailable())
    {
        return;
    }
    if (!shader.loadFromMemory(FragmentShaderSource, sf::Shader::Type::Fragment))
    {
        throw ResourceError("Could not compile the drone camera shader. SFML printed the GLSL error above.");
    }
    shader.setUniform("scene", sf::Shader::CurrentTexture);
    shader.setUniform("resolution", sf::Glsl::Vec2(sf::Vector2f(windowSize)));
    shaderAvailable = true;
}

sf::RenderTexture& DroneCameraFilter::getSceneTexture()
{
    return sceneTexture;
}

void DroneCameraFilter::drawToWindow(sf::RenderTarget& window, float elapsedSeconds)
{
    sceneTexture.display();
    sf::Sprite scene(sceneTexture.getTexture());
    window.setView(window.getDefaultView());
    if (!shaderAvailable)
    {
        window.draw(scene);
        return;
    }

    shader.setUniform("time", elapsedSeconds);
    window.draw(scene, &shader);
}

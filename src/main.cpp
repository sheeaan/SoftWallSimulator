#include "presentation/Game.hpp"
#include "simulation/SimulatorErrors.hpp"

#include <iostream>

int main()
{
    try
    {
        Game game;
        game.run();
    }
    catch (const SimulatorError& error)
    {
        std::cerr << "Soft Wall Simulator stopped: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

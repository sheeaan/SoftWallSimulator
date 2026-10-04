#pragma once

#include <stdexcept>
#include <string>

// Every error the simulator raises derives from SimulatorError, so main can catch one type.
class SimulatorError : public std::runtime_error
{
public:
    explicit SimulatorError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

// A grid was given an invalid size, or a cell outside the grid was read or written.
class GridBoundsError : public SimulatorError
{
public:
    explicit GridBoundsError(const std::string& message)
        : SimulatorError(message)
    {
    }
};

// An ASCII level has rows of different widths, an unknown character, or the wrong number of spawns.
class LevelFormatError : public SimulatorError
{
public:
    explicit LevelFormatError(const std::string& message)
        : SimulatorError("Invalid level: " + message)
    {
    }
};

// A font or a render texture could not be created.
class ResourceError : public SimulatorError
{
public:
    explicit ResourceError(const std::string& message)
        : SimulatorError(message)
    {
    }
};

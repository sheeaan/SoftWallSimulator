#include "simulation/Enemy.hpp"

#include <utility>

Enemy::Enemy(sf::Vector2f spawnPosition)
    : position(spawnPosition)
{
}

void Enemy::setPath(std::vector<GridCell> newPath)
{
    path = std::move(newPath);
    if (path.size() > 1)
    {
        nextWaypointIndex = 1;
    }
    else
    {
        nextWaypointIndex = 0;
    }
}

void Enemy::followPath(float seconds)
{
    // Spend this frame's travel distance on the waypoints in order.
    // Reaching a waypoint carries the leftover distance on to the next one, so the speed stays constant.
    float distanceLeft = Speed * seconds;
    while (distanceLeft > 0.f && nextWaypointIndex < path.size())
    {
        sf::Vector2f waypoint = cellCenter(path[nextWaypointIndex]);
        sf::Vector2f toWaypoint = waypoint - position;
        float distanceToWaypoint = toWaypoint.length();
        if (distanceToWaypoint <= distanceLeft)
        {
            position = waypoint;
            distanceLeft -= distanceToWaypoint;
            nextWaypointIndex += 1;
        }
        else
        {
            position += toWaypoint / distanceToWaypoint * distanceLeft;
            distanceLeft = 0.f;
        }
    }
}

sf::Vector2f Enemy::getPosition() const
{
    return position;
}

GridCell Enemy::getCell() const
{
    return cellContaining(position);
}

const std::vector<GridCell>& Enemy::getPath() const
{
    return path;
}

std::size_t Enemy::getNextWaypointIndex() const
{
    return nextWaypointIndex;
}

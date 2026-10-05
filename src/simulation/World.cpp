#include "simulation/World.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <utility>

namespace
{
    float millisecondsSince(std::chrono::steady_clock::time_point start)
    {
        std::chrono::duration<float, std::milli> elapsed = std::chrono::steady_clock::now() - start;
        return elapsed.count();
    }

    // Which tiles each charge can open. A breach charge cannot touch steel; a hard breach charge burns through it.
    bool canChargeOpen(ChargeType type, TileType tile)
    {
        if (type == ChargeType::BREACH)
        {
            return tile == TileType::SOFT_WALL || tile == TileType::BARRICADE;
        }
        return tile == TileType::SOFT_WALL || tile == TileType::REINFORCED_WALL;
    }

    float fuseSeconds(ChargeType type)
    {
        if (type == ChargeType::BREACH)
        {
            return World::BreachFuseSeconds;
        }
        return World::HardBreachFuseSeconds;
    }
}

World::World(const Level& level)
    : grid(level.grid)
    , player(cellCenter(level.playerSpawn))
    , enemy(cellCenter(level.enemySpawn))
    , round(!level.bombSite.empty())
    , bombSite(level.bombSite)
    , guardPoints(level.guardPoints)
    , plannedReinforcements(level.plannedReinforcements)
{
    // Without a bomb site there is no round, and the defender hunts from the first frame.
    if (round.getPhase() == RoundPhase::FREE_PLAY)
    {
        defenderState = DefenderState::HUNTING;
        setEnemyGoal(player.getCell());
    }
    else
    {
        defenderState = DefenderState::PREPARING;
        startNextReinforcement();
    }
    castVisibility();
}

void World::update(float seconds, sf::Vector2f inputDirection)
{
    detonations.clear();
    reinforcedCells.clear();
    if (round.isOver())
    {
        return; // The round is decided. Everything holds still until the next round.
    }

    // The attacker drives the drone, or walks. During preparation the attacker waits outside; only the drone moves.
    if (drivingDrone && drone.has_value())
    {
        drone.value().move(inputDirection, seconds, grid);
    }
    else if (round.getPhase() != RoundPhase::PREPARATION)
    {
        player.move(inputDirection, seconds, grid);
    }

    burnFuses(seconds);
    updateDefender(seconds);
    castVisibility();
    updateSpotting(seconds);
    round.update(seconds, isInBombSite(player.getCell()));
}

DamageResult World::damageTileAt(sf::Vector2f worldPosition)
{
    // No shooting before the action phase, after the round, or while driving the drone.
    bool phaseAllowsShooting = round.getPhase() == RoundPhase::ACTION || round.getPhase() == RoundPhase::FREE_PLAY;
    if (!phaseAllowsShooting || drivingDrone)
    {
        return DamageResult::NO_EFFECT;
    }

    // The shot flies from the player to the clicked point and drills its slot through the wall there.
    GridCell cell = cellContaining(worldPosition);
    std::uint16_t panelsHit = panelsAlongShot(cell, player.getPosition(), worldPosition);
    DamageResult result = grid.damageTile(cell, panelsHit);
    if (result == DamageResult::DESTROYED)
    {
        statistics.wallsDestroyed += 1;

        // The map's shape just changed, so the old path may be blocked or no longer the shortest.
        recalculateEnemyPath();
    }
    return result;
}

ChargePlacement World::placeCharge(ChargeType type, sf::Vector2f worldPosition)
{
    bool phaseAllowsBreaching = round.getPhase() == RoundPhase::ACTION || round.getPhase() == RoundPhase::FREE_PLAY;
    if (!phaseAllowsBreaching || drivingDrone)
    {
        return ChargePlacement::NOT_NOW;
    }
    if (getChargesLeft(type) == 0)
    {
        return ChargePlacement::NONE_LEFT;
    }

    // The wall must touch the attacker's cell, sides and corners included.
    GridCell wall = cellContaining(worldPosition);
    GridCell playerCell = player.getCell();
    int cellsApart = std::max(std::abs(wall.column - playerCell.column), std::abs(wall.row - playerCell.row));
    if (cellsApart > 1)
    {
        return ChargePlacement::TOO_FAR;
    }
    if (!grid.contains(wall) || !canChargeOpen(type, grid.getTile(wall).type))
    {
        return ChargePlacement::WRONG_SURFACE;
    }
    for (const Charge& placedCharge : charges)
    {
        if (placedCharge.wall == wall)
        {
            return ChargePlacement::ALREADY_CHARGED;
        }
    }

    Charge charge;
    charge.type = type;
    charge.wall = wall;
    charge.secondsLeft = fuseSeconds(type);
    charges.push_back(charge);
    if (type == ChargeType::BREACH)
    {
        breachChargesLeft -= 1;
    }
    else
    {
        hardBreachChargesLeft -= 1;
    }
    return ChargePlacement::PLACED;
}

void World::toggleDrone()
{
    if (round.isOver())
    {
        return;
    }
    if (!drone.has_value())
    {
        drone = Drone(player.getPosition());
        drivingDrone = true;
        return;
    }
    drivingDrone = !drivingDrone;
}

const Grid& World::getGrid() const
{
    return grid;
}

const Player& World::getPlayer() const
{
    return player;
}

const Enemy& World::getEnemy() const
{
    return enemy;
}

const std::optional<Drone>& World::getDrone() const
{
    return drone;
}

bool World::isDrivingDrone() const
{
    return drivingDrone;
}

const std::vector<RayHit>& World::getVisibility() const
{
    return raycaster.getVisibility();
}

const std::vector<RayHit>& World::getDroneVisibility() const
{
    if (drivingDrone)
    {
        return droneRaycaster.getVisibility();
    }
    return noVisibility;
}

const Pathfinder& World::getPathfinder() const
{
    return pathfinder;
}

const SimulationStatistics& World::getStatistics() const
{
    return statistics;
}

const Round& World::getRound() const
{
    return round;
}

const std::vector<GridCell>& World::getBombSite() const
{
    return bombSite;
}

const std::vector<Charge>& World::getCharges() const
{
    return charges;
}

int World::getChargesLeft(ChargeType type) const
{
    if (type == ChargeType::BREACH)
    {
        return breachChargesLeft;
    }
    return hardBreachChargesLeft;
}

DefenderState World::getDefenderState() const
{
    return defenderState;
}

const std::vector<Detonation>& World::getDetonations() const
{
    return detonations;
}

const std::vector<GridCell>& World::getReinforcedCells() const
{
    return reinforcedCells;
}

bool World::canPlayerSeeEnemy() const
{
    return raycaster.hasLineOfSight(player.getPosition(), enemy.getPosition(), grid);
}

bool World::canDroneSeeEnemy() const
{
    if (!drivingDrone || !drone.has_value())
    {
        return false;
    }
    return droneRaycaster.hasLineOfSight(drone.value().getPosition(), enemy.getPosition(), grid);
}

bool World::isEnemySpotted() const
{
    return spottedSecondsLeft > 0.f;
}

RayTrace World::traceRayFromPlayer(sf::Vector2f toward) const
{
    sf::Vector2f offset = toward - player.getPosition();
    sf::Vector2f direction(1.f, 0.f); // A point on top of the player has no direction; look east.
    if (offset.length() > 0.001f)
    {
        direction = offset.normalized();
    }
    return raycaster.traceRay(player.getPosition(), direction, grid);
}

void World::castVisibility()
{
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    raycaster.castVisibility(player.getPosition(), grid);
    statistics.visibilityMilliseconds = millisecondsSince(start);

    if (drivingDrone && drone.has_value())
    {
        droneRaycaster.castVisibility(drone.value().getPosition(), grid);
    }
}

void World::updateSpotting(float seconds)
{
    // Whatever the attacker or the drone sees is spotted, and the mark lingers for a few seconds.
    bool seen = canPlayerSeeEnemy() || canDroneSeeEnemy();
    if (seen)
    {
        spottedSecondsLeft = SpottedSeconds;
    }
    else
    {
        spottedSecondsLeft = std::max(0.f, spottedSecondsLeft - seconds);
    }
}

void World::burnFuses(float seconds)
{
    bool mapChanged = false;
    std::size_t index = 0;
    while (index < charges.size())
    {
        charges[index].secondsLeft -= seconds;
        if (charges[index].secondsLeft > 0.f)
        {
            index += 1;
            continue;
        }

        Detonation detonation = detonate(charges[index]);
        if (!detonation.destroyedCells.empty())
        {
            mapChanged = true;
            statistics.wallsDestroyed += static_cast<int>(detonation.destroyedCells.size());
        }
        detonations.push_back(detonation);

        // Swap and pop. The moved charge has not burned this frame yet, so the index stays.
        charges[index] = charges.back();
        charges.pop_back();
    }

    // One re-plan covers every tile the charges opened this frame.
    if (mapChanged)
    {
        recalculateEnemyPath();
    }
}

Detonation World::detonate(const Charge& charge)
{
    Detonation detonation;
    detonation.type = charge.type;
    detonation.wall = charge.wall;

    // If the wall is already gone, the charge falls to the floor and goes off harmlessly.
    if (!grid.contains(charge.wall) || !canChargeOpen(charge.type, grid.getTile(charge.wall).type))
    {
        return detonation;
    }

    // The blast opens the wall and each neighbor, up, down, left, and right, that the same charge could open.
    const GridCell blastCells[5] = {
        charge.wall,
        GridCell{charge.wall.column, charge.wall.row - 1},
        GridCell{charge.wall.column, charge.wall.row + 1},
        GridCell{charge.wall.column - 1, charge.wall.row},
        GridCell{charge.wall.column + 1, charge.wall.row},
    };
    for (const GridCell& cell : blastCells)
    {
        if (grid.contains(cell) && canChargeOpen(charge.type, grid.getTile(cell).type) && grid.destroyTile(cell))
        {
            detonation.destroyedCells.push_back(cell);
        }
    }
    return detonation;
}

void World::updateDefender(float seconds)
{
    // When preparation ends, unfinished reinforcements are abandoned and the defender takes the site.
    if (round.getPhase() == RoundPhase::ACTION && defenderState == DefenderState::PREPARING)
    {
        startGuarding();
    }

    // In the action phase, the defender starts hunting the moment it sees the attacker.
    if (round.getPhase() == RoundPhase::ACTION && defenderState != DefenderState::HUNTING && canEnemySeePlayer())
    {
        defenderState = DefenderState::HUNTING;
        setEnemyGoal(player.getCell());
    }

    if (defenderState == DefenderState::PREPARING)
    {
        updatePreparing(seconds);
    }
    else if (defenderState == DefenderState::GUARDING)
    {
        updateGuarding(seconds);
    }
    else if (player.getCell() != enemyGoal)
    {
        // Hunting. Within one cell the old path still leads to the attacker, so re-plan only when the attacker changes cell.
        setEnemyGoal(player.getCell());
    }

    enemy.followPath(seconds);

    // The defender caught the attacker.
    float distanceToAttacker = (enemy.getPosition() - player.getPosition()).length();
    if (defenderState == DefenderState::HUNTING && distanceToAttacker < CatchDistance)
    {
        round.eliminateAttacker();
    }
}

void World::updatePreparing(float seconds)
{
    if (reinforceSecondsLeft > 0.f)
    {
        // Fixing the plate. When the time is up, the wall turns to steel and the defender moves on.
        reinforceSecondsLeft -= seconds;
        if (reinforceSecondsLeft <= 0.f && nextReinforcement < plannedReinforcements.size())
        {
            GridCell wall = plannedReinforcements[nextReinforcement];
            if (grid.reinforceTile(wall))
            {
                reinforcedCells.push_back(wall);
            }
            nextReinforcement += 1;
            startNextReinforcement();
        }
        return;
    }

    // Walking to the wall. Once there, start fixing the plate.
    if (enemy.hasFinishedPath())
    {
        reinforceSecondsLeft = ReinforceSeconds;
    }
}

void World::updateGuarding(float seconds)
{
    // Stand at a guard point for a few seconds, then walk to the next one.
    bool arrived = enemy.hasFinishedPath() || enemy.getPath().empty();
    if (guardPoints.empty() || !arrived)
    {
        return;
    }
    guardPauseSecondsLeft -= seconds;
    if (guardPauseSecondsLeft > 0.f)
    {
        return;
    }
    guardPauseSecondsLeft = GuardPauseSeconds;
    guardPointIndex = (guardPointIndex + 1) % guardPoints.size();
    setEnemyGoal(guardPoints[guardPointIndex]);
}

void World::startNextReinforcement()
{
    // Skip planned walls that are already gone or already steel.
    while (nextReinforcement < plannedReinforcements.size())
    {
        GridCell wall = plannedReinforcements[nextReinforcement];
        if (grid.contains(wall) && grid.getTile(wall).type == TileType::SOFT_WALL)
        {
            // Stand on whichever open side of the wall the defender can reach in the fewest steps.
            const GridCell sides[4] = {
                GridCell{wall.column, wall.row - 1},
                GridCell{wall.column, wall.row + 1},
                GridCell{wall.column - 1, wall.row},
                GridCell{wall.column + 1, wall.row},
            };
            std::optional<GridCell> bestSide;
            std::size_t bestPathLength = 0;
            for (const GridCell& side : sides)
            {
                if (!grid.isWalkable(side))
                {
                    continue;
                }
                std::vector<GridCell> path = pathfinder.findPath(grid, enemy.getCell(), side);
                if (path.empty())
                {
                    continue;
                }
                if (!bestSide.has_value() || path.size() < bestPathLength)
                {
                    bestSide = side;
                    bestPathLength = path.size();
                }
            }
            if (bestSide.has_value())
            {
                setEnemyGoal(bestSide.value());
                return;
            }
        }
        nextReinforcement += 1;
    }

    // Nothing left to reinforce: take the site.
    startGuarding();
}

void World::startGuarding()
{
    defenderState = DefenderState::GUARDING;
    reinforceSecondsLeft = 0.f;
    guardPointIndex = 0;
    guardPauseSecondsLeft = GuardPauseSeconds;
    if (!guardPoints.empty())
    {
        setEnemyGoal(guardPoints[guardPointIndex]);
    }
}

void World::setEnemyGoal(GridCell goal)
{
    enemyGoal = goal;
    recalculateEnemyPath();
}

void World::recalculateEnemyPath()
{
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::vector<GridCell> path = pathfinder.findPath(grid, enemy.getCell(), enemyGoal);
    statistics.pathfindingMilliseconds = millisecondsSince(start);
    statistics.pathRecalculations += 1;

    enemy.setPath(std::move(path));
}

bool World::canEnemySeePlayer() const
{
    return raycaster.hasLineOfSight(enemy.getPosition(), player.getPosition(), grid);
}

bool World::isInBombSite(GridCell cell) const
{
    for (const GridCell& siteCell : bombSite)
    {
        if (siteCell == cell)
        {
            return true;
        }
    }
    return false;
}

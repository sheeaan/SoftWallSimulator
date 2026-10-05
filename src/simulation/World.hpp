#pragma once

#include "simulation/Drone.hpp"
#include "simulation/Enemy.hpp"
#include "simulation/Grid.hpp"
#include "simulation/Level.hpp"
#include "simulation/Pathfinder.hpp"
#include "simulation/Player.hpp"
#include "simulation/Raycaster.hpp"
#include "simulation/Round.hpp"

#include <SFML/System/Vector2.hpp>

#include <cstddef>
#include <optional>
#include <vector>

// Numbers the statistics panel shows that the simulation cannot rebuild from its current state.
struct SimulationStatistics
{
    float visibilityMilliseconds = 0.f;  // The last 360-ray cast.
    float pathfindingMilliseconds = 0.f; // The last A* search.
    int pathRecalculations = 0;
    int wallsDestroyed = 0;
};

enum class ChargeType
{
    BREACH,    // Breach charge: blows out a soft wall or barricade and its soft neighbors. Useless on steel.
    EXOTHERMIC // Thermite's exothermic charge: burns through reinforced walls as well as drywall.
};

enum class ChargePlacement
{
    PLACED,
    NONE_LEFT,
    TOO_FAR,         // The wall must touch the attacker's cell.
    WRONG_SURFACE,   // Not a wall this charge can open.
    ALREADY_CHARGED, // That wall already carries a charge.
    NOT_NOW          // During the preparation phase, while driving the drone, or after the round.
};

// A charge stuck on a wall, burning down its fuse.
struct Charge
{
    ChargeType type = ChargeType::BREACH;
    GridCell wall;
    float secondsLeft = 0.f;
};

// What one charge blew open, so the presentation can show the explosion.
struct Detonation
{
    ChargeType type = ChargeType::BREACH;
    GridCell wall;
    std::vector<GridCell> destroyedCells;
};

enum class DefenderState
{
    PREPARING, // Walking to the planned walls and reinforcing them.
    GUARDING,  // Holding the bomb site, moving between its guard points.
    HUNTING    // Has seen the attacker and chases them.
};

// The whole simulation. The presentation layer drives it through update() and the attacker's actions,
// and reads it through const getters.
class World
{
public:
    static constexpr int BreachChargeCount = 3;
    static constexpr int ExothermicChargeCount = 2;
    static constexpr float BreachFuseSeconds = 1.5f;
    static constexpr float ExothermicFuseSeconds = 3.f;
    static constexpr float ReinforceSeconds = 1.2f;
    static constexpr float SpottedSeconds = 4.f;
    static constexpr float GuardPauseSeconds = 3.f;
    static constexpr float CatchDistance = 0.6f; // Tiles between defender and attacker that end the round.

    explicit World(const Level& level);

    // Moves whatever the attacker controls, burns the fuses, runs the defender, and advances the round.
    void update(float seconds, sf::Vector2f inputDirection);

    // Shoots the tile under a world position from the player's position. A soft wall loses one health
    // and gets a see-through slot along the line of fire; a barricade breaks. When a tile breaks, the enemy
    // re-plans inside this same call, so the new path exists before the next frame is drawn.
    DamageResult damageTileAt(sf::Vector2f worldPosition);

    // Sticks a charge on the wall under a world position. The wall must touch the attacker's cell.
    ChargePlacement placeCharge(ChargeType type, sf::Vector2f worldPosition);

    // Deploys the drone at the attacker's feet, or switches control between the drone and the attacker.
    void toggleDrone();

    const Grid& getGrid() const;
    const Player& getPlayer() const;
    const Enemy& getEnemy() const;
    const std::optional<Drone>& getDrone() const;
    bool isDrivingDrone() const;
    const std::vector<RayHit>& getVisibility() const;
    const std::vector<RayHit>& getDroneVisibility() const; // Empty unless the attacker is driving the drone.
    const Pathfinder& getPathfinder() const;
    const SimulationStatistics& getStatistics() const;
    const Round& getRound() const;
    const std::vector<GridCell>& getBombSite() const;
    const std::vector<Charge>& getCharges() const;
    int getChargesLeft(ChargeType type) const;
    DefenderState getDefenderState() const;

    // What happened during the last update, for effects.
    const std::vector<Detonation>& getDetonations() const;
    const std::vector<GridCell>& getReinforcedCells() const;

    bool canPlayerSeeEnemy() const;
    bool canDroneSeeEnemy() const; // False unless the attacker is driving the drone.
    bool isEnemySpotted() const;   // Seen by the attacker or the drone within the last few seconds.

    // The full DDA record of one ray from the player toward a point, for the math view.
    RayTrace traceRayFromPlayer(sf::Vector2f toward) const;

private:
    void castVisibility();
    void updateSpotting(float seconds);
    void burnFuses(float seconds);
    Detonation detonate(const Charge& charge);
    void updateDefender(float seconds);
    void updatePreparing(float seconds);
    void updateGuarding(float seconds);
    void startNextReinforcement();
    void startGuarding();
    void setEnemyGoal(GridCell goal);
    void recalculateEnemyPath();
    bool canEnemySeePlayer() const;
    bool isInBombSite(GridCell cell) const;

    Grid grid;
    Player player;
    Enemy enemy;
    std::optional<Drone> drone;
    bool drivingDrone = false;
    Raycaster raycaster;
    Raycaster droneRaycaster;
    Pathfinder pathfinder;
    Round round;
    SimulationStatistics statistics;

    std::vector<GridCell> bombSite;
    std::vector<GridCell> guardPoints;
    std::vector<GridCell> plannedReinforcements;

    std::vector<Charge> charges;
    int breachChargesLeft = BreachChargeCount;
    int exothermicChargesLeft = ExothermicChargeCount;
    std::vector<Detonation> detonations;
    std::vector<GridCell> reinforcedCells;
    std::vector<RayHit> noVisibility; // Always empty: the drone's visibility while nobody drives it.

    DefenderState defenderState = DefenderState::HUNTING;
    GridCell enemyGoal;                    // The cell the defender's current path leads to.
    std::size_t nextReinforcement = 0;     // Index into plannedReinforcements.
    float reinforceSecondsLeft = 0.f;      // Above zero while the defender is fixing a plate to a wall.
    std::size_t guardPointIndex = 0;       // Index into guardPoints.
    float guardPauseSecondsLeft = 0.f;
    float spottedSecondsLeft = 0.f;
};

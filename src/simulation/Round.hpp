#pragma once

enum class RoundPhase
{
    FREE_PLAY,     // The level has no bomb site: no clock, no win or loss.
    PREPARATION,   // The defender reinforces. The attacker can only drive the drone.
    ACTION,        // The attacker moves in, and the clock runs.
    ATTACKERS_WIN,
    DEFENDERS_WIN
};

enum class RoundEndReason
{
    NONE,
    DEFUSER_PLANTED,
    ATTACKER_ELIMINATED,
    TIME_EXPIRED
};

// The clock and the rules of one round of Siege's bomb mode.
class Round
{
public:
    static constexpr float PreparationSeconds = 20.f;
    static constexpr float ActionSeconds = 180.f;
    static constexpr float PlantSeconds = 4.f;

    explicit Round(bool hasObjective);

    // Advances the clock. While the attacker stands in the bomb site during the action phase,
    // the defuser plants. Stepping out of the site starts the plant over.
    void update(float seconds, bool attackerInBombSite);

    // The defender caught the attacker. Ends an action phase; does nothing in free play.
    void eliminateAttacker();

    RoundPhase getPhase() const;
    RoundEndReason getEndReason() const;
    float getSecondsLeft() const;   // In the current phase.
    float getPlantProgress() const; // From 0 to 1.
    bool isOver() const;

private:
    RoundPhase phase;
    RoundEndReason endReason = RoundEndReason::NONE;
    float secondsLeft = 0.f;
    float plantedSeconds = 0.f;
};

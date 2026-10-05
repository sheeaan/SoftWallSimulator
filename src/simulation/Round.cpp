#include "simulation/Round.hpp"

Round::Round(bool hasObjective)
    : phase(RoundPhase::FREE_PLAY)
{
    if (hasObjective)
    {
        phase = RoundPhase::PREPARATION;
        secondsLeft = PreparationSeconds;
    }
}

void Round::update(float seconds, bool attackerInBombSite)
{
    if (phase == RoundPhase::PREPARATION)
    {
        secondsLeft -= seconds;
        if (secondsLeft <= 0.f)
        {
            phase = RoundPhase::ACTION;
            secondsLeft = ActionSeconds;
        }
        return;
    }
    if (phase != RoundPhase::ACTION)
    {
        return;
    }

    if (attackerInBombSite)
    {
        plantedSeconds += seconds;
    }
    else
    {
        plantedSeconds = 0.f;
    }
    if (plantedSeconds >= PlantSeconds)
    {
        phase = RoundPhase::ATTACKERS_WIN;
        endReason = RoundEndReason::DEFUSER_PLANTED;
        return;
    }

    secondsLeft -= seconds;
    if (secondsLeft <= 0.f)
    {
        secondsLeft = 0.f;
        phase = RoundPhase::DEFENDERS_WIN;
        endReason = RoundEndReason::TIME_EXPIRED;
    }
}

void Round::eliminateAttacker()
{
    if (phase != RoundPhase::ACTION)
    {
        return;
    }
    phase = RoundPhase::DEFENDERS_WIN;
    endReason = RoundEndReason::ATTACKER_ELIMINATED;
}

RoundPhase Round::getPhase() const
{
    return phase;
}

RoundEndReason Round::getEndReason() const
{
    return endReason;
}

float Round::getSecondsLeft() const
{
    return secondsLeft;
}

float Round::getPlantProgress() const
{
    return plantedSeconds / PlantSeconds;
}

bool Round::isOver() const
{
    return phase == RoundPhase::ATTACKERS_WIN || phase == RoundPhase::DEFENDERS_WIN;
}

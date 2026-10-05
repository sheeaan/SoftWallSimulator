#include "presentation/HeadsUpDisplay.hpp"

#include "presentation/Fonts.hpp"
#include "presentation/Palette.hpp"
#include "presentation/Textures.hpp"

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/String.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace
{
    constexpr float MessageSeconds = 2.2f;

    // Centers a text on a point, using the text's own bounds.
    void centerText(sf::Text& text, sf::Vector2f center)
    {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.position + bounds.size / 2.f);
        text.setPosition(center);
    }

    // The × in the key prompts is UTF-8 in the source. sf::Text wants UTF-32.
    sf::String fromUtf8(const std::string& text)
    {
        return sf::String::fromUtf8(text.begin(), text.end());
    }

    // Minutes and seconds, rounded up, the way a round clock counts down: 2:41.
    std::string formatClock(float secondsLeft)
    {
        int totalSeconds = static_cast<int>(std::ceil(std::max(0.f, secondsLeft)));
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;
        std::string secondsText = std::to_string(seconds);
        if (seconds < 10)
        {
            secondsText = "0" + secondsText;
        }
        return std::to_string(minutes) + ":" + secondsText;
    }

    struct KeyPrompt
    {
        std::string key;
        std::string action;
    };

    // Scales a sprite to a square of the given size and centers it on a point.
    void placeBadge(sf::Sprite& badge, float size, sf::Vector2f center)
    {
        sf::Vector2f textureSize(badge.getTexture().getSize());
        badge.setScale(sf::Vector2f(size / textureSize.x, size / textureSize.y));
        badge.setPosition(center - sf::Vector2f(size / 2.f, size / 2.f));
    }
}

HeadsUpDisplay::HeadsUpDisplay(sf::Vector2u windowPixelSize)
    : font(loadFont("BarlowCondensed-SemiBold.ttf"))
    , strikerBadge(loadSiegeTexture("striker.png"))
    , sentryBadge(loadSiegeTexture("sentry.png"))
    , windowSize(windowPixelSize)
    , unit(static_cast<float>(windowPixelSize.y) / 100.f)
    , strikerSprite(strikerBadge)
    , sentrySprite(sentryBadge)
    , attackerText(font, "STRIKER", static_cast<unsigned int>(unit * 2.4f))
    , clockText(font, "", static_cast<unsigned int>(unit * 2.4f))
    , defenderText(font, "SENTRY", static_cast<unsigned int>(unit * 2.4f))
    , statusText(font, "", static_cast<unsigned int>(unit * 2.1f))
    , messageText(font, "", static_cast<unsigned int>(unit * 2.f))
    , resultBadge(strikerBadge)
    , resultTitle(font, "", static_cast<unsigned int>(unit * 6.f))
    , resultReason(font, "", static_cast<unsigned int>(unit * 2.6f))
    , resultHint(font, "PRESS R FOR THE NEXT ROUND", static_cast<unsigned int>(unit * 2.f))
{
    // The banner: blue attacker block, dark clock block, orange defender block, centered at the top.
    float teamWidth = unit * 15.f;
    float clockWidth = unit * 22.f;
    float bannerHeight = unit * 4.4f;
    float bannerLeft = (windowSize.x - (teamWidth * 2.f + clockWidth)) / 2.f;
    float bannerTop = unit * 1.6f;
    float bannerMiddle = bannerTop + bannerHeight / 2.f;

    attackerBlock.setSize(sf::Vector2f(teamWidth, bannerHeight));
    attackerBlock.setPosition(sf::Vector2f(bannerLeft, bannerTop));
    attackerBlock.setFillColor(Palette::AttackerBlue);
    clockBlock.setSize(sf::Vector2f(clockWidth, bannerHeight));
    clockBlock.setPosition(sf::Vector2f(bannerLeft + teamWidth, bannerTop));
    clockBlock.setFillColor(Palette::HudBackground);
    defenderBlock.setSize(sf::Vector2f(teamWidth, bannerHeight));
    defenderBlock.setPosition(sf::Vector2f(bannerLeft + teamWidth + clockWidth, bannerTop));
    defenderBlock.setFillColor(Palette::DefenderOrange);

    attackerText.setFillColor(Palette::HudText);
    centerText(attackerText, sf::Vector2f(bannerLeft + teamWidth / 2.f, bannerMiddle));
    defenderText.setFillColor(Palette::HudText);
    centerText(defenderText, sf::Vector2f(bannerLeft + teamWidth * 1.5f + clockWidth, bannerMiddle));
    clockText.setFillColor(Palette::HudText);
    clockText.setPosition(sf::Vector2f(windowSize.x / 2.f, bannerMiddle));

    // The official operator icons flank the banner, as in Siege's top bar.
    float badgeSize = unit * 6.6f;
    float badgeGap = unit * 0.6f;
    placeBadge(strikerSprite, badgeSize, sf::Vector2f(bannerLeft - badgeGap - badgeSize / 2.f, bannerMiddle));
    placeBadge(sentrySprite, badgeSize,
               sf::Vector2f(bannerLeft + teamWidth * 2.f + clockWidth + badgeGap + badgeSize / 2.f, bannerMiddle));

    // The planting bar sits under the status line.
    float barWidth = clockWidth;
    plantBarBackground.setSize(sf::Vector2f(barWidth, unit * 0.8f));
    plantBarBackground.setPosition(sf::Vector2f((windowSize.x - barWidth) / 2.f, unit * 10.2f));
    plantBarBackground.setFillColor(Palette::HudBackground);
    plantBarFill.setPosition(plantBarBackground.getPosition());
    plantBarFill.setFillColor(Palette::AttackerBlue);

    messageText.setFillColor(Palette::HudMessage);

    // The result panel fills the middle of the screen when a round ends: the winner's icon, then the words.
    sf::Vector2f resultSize(unit * 52.f, unit * 28.f);
    resultBackground.setSize(resultSize);
    resultBackground.setPosition((windowSize - resultSize) / 2.f);
    resultBackground.setFillColor(Palette::ResultBackground);
    resultReason.setFillColor(Palette::HudText);
    resultHint.setFillColor(Palette::HudMutedText);
    centerText(resultHint, sf::Vector2f(windowSize.x / 2.f, windowSize.y / 2.f + unit * 10.5f));

    layOutPrompts(World::BreachChargeCount, World::HardBreachChargeCount);
}

void HeadsUpDisplay::update(const World& world, float seconds)
{
    const Round& round = world.getRound();
    sf::Vector2f clockCenter(windowSize.x / 2.f, unit * 1.6f + unit * 2.2f);

    std::string clock = "FREE PLAY";
    if (round.getPhase() == RoundPhase::PREPARATION)
    {
        clock = "PREPARATION   " + formatClock(round.getSecondsLeft());
    }
    else if (round.getPhase() == RoundPhase::ACTION)
    {
        clock = "ACTION   " + formatClock(round.getSecondsLeft());
    }
    else if (round.isOver())
    {
        clock = "ROUND OVER";
    }
    clockText.setString(clock);
    centerText(clockText, clockCenter);

    // The status line: planting beats everything, then the phase, then whether the defender is spotted.
    planting = round.getPhase() == RoundPhase::ACTION && round.getPlantProgress() > 0.f;
    if (planting)
    {
        statusText.setString("PLANTING DEFUSER");
        statusText.setFillColor(Palette::HudText);
        plantBarFill.setSize(sf::Vector2f(plantBarBackground.getSize().x * std::min(1.f, round.getPlantProgress()),
                                          plantBarBackground.getSize().y));
    }
    else if (round.getPhase() == RoundPhase::PREPARATION)
    {
        statusText.setString("SENTRY IS REINFORCING.  PRESS F TO DRONE IN.");
        statusText.setFillColor(Palette::HudMutedText);
    }
    else if (world.isEnemySpotted())
    {
        statusText.setString("DEFENDER SPOTTED");
        statusText.setFillColor(Palette::DefenderOrange);
    }
    else
    {
        statusText.setString("DEFENDER HIDDEN");
        statusText.setFillColor(Palette::HudMutedText);
    }
    centerText(statusText, sf::Vector2f(windowSize.x / 2.f, unit * 8.4f));

    messageSecondsLeft = std::max(0.f, messageSecondsLeft - seconds);

    roundOver = round.isOver();
    if (roundOver)
    {
        if (round.getPhase() == RoundPhase::ATTACKERS_WIN)
        {
            resultTitle.setString("ATTACKERS WIN");
            resultTitle.setFillColor(Palette::AttackerBlue);
            resultBadge.setTexture(strikerBadge, true);
        }
        else
        {
            resultTitle.setString("DEFENDERS WIN");
            resultTitle.setFillColor(Palette::DefenderOrange);
            resultBadge.setTexture(sentryBadge, true);
        }
        placeBadge(resultBadge, unit * 9.f, sf::Vector2f(windowSize.x / 2.f, windowSize.y / 2.f - unit * 7.5f));

        std::string reason;
        switch (round.getEndReason())
        {
        case RoundEndReason::DEFUSER_PLANTED:
            reason = "THE DEFUSER IS PLANTED";
            break;
        case RoundEndReason::ATTACKER_ELIMINATED:
            reason = "STRIKER WAS CAUGHT";
            break;
        case RoundEndReason::TIME_EXPIRED:
            reason = "TIME RAN OUT";
            break;
        case RoundEndReason::NONE:
            break;
        }
        resultReason.setString(reason);
        centerText(resultTitle, sf::Vector2f(windowSize.x / 2.f, windowSize.y / 2.f + unit * 1.f));
        centerText(resultReason, sf::Vector2f(windowSize.x / 2.f, windowSize.y / 2.f + unit * 6.f));
    }

    int breachChargesLeft = world.getChargesLeft(ChargeType::BREACH);
    int hardBreachChargesLeft = world.getChargesLeft(ChargeType::HARD_BREACH);
    if (breachChargesLeft != promptedBreachCharges || hardBreachChargesLeft != promptedHardBreachCharges)
    {
        layOutPrompts(breachChargesLeft, hardBreachChargesLeft);
    }
}

void HeadsUpDisplay::showMessage(const std::string& message)
{
    messageText.setString(message);
    centerText(messageText, sf::Vector2f(windowSize.x / 2.f, unit * 12.8f));
    messageSecondsLeft = MessageSeconds;
}

void HeadsUpDisplay::draw(sf::RenderTarget& target) const
{
    // Laid out in pixels, so it draws with the window's default view.
    sf::View previousView = target.getView();
    target.setView(target.getDefaultView());

    target.draw(attackerBlock);
    target.draw(clockBlock);
    target.draw(defenderBlock);
    target.draw(strikerSprite);
    target.draw(sentrySprite);
    target.draw(attackerText);
    target.draw(clockText);
    target.draw(defenderText);
    target.draw(statusText);
    if (planting)
    {
        target.draw(plantBarBackground);
        target.draw(plantBarFill);
    }
    if (messageSecondsLeft > 0.f)
    {
        // Fade out over the last half second.
        sf::Text fadingMessage = messageText;
        sf::Color color = Palette::HudMessage;
        color.a = static_cast<std::uint8_t>(255.f * std::min(1.f, messageSecondsLeft / 0.5f));
        fadingMessage.setFillColor(color);
        target.draw(fadingMessage);
    }

    for (std::size_t promptIndex = 0; promptIndex < keycaps.size(); ++promptIndex)
    {
        target.draw(keycaps[promptIndex]);
        target.draw(keyTexts[promptIndex]);
        target.draw(actionTexts[promptIndex]);
    }

    if (roundOver)
    {
        target.draw(resultBackground);
        target.draw(resultBadge);
        target.draw(resultTitle);
        target.draw(resultReason);
        target.draw(resultHint);
    }

    target.setView(previousView);
}

void HeadsUpDisplay::layOutPrompts(int breachChargesLeft, int hardBreachChargesLeft)
{
    promptedBreachCharges = breachChargesLeft;
    promptedHardBreachCharges = hardBreachChargesLeft;
    keycaps.clear();
    keyTexts.clear();
    actionTexts.clear();

    // Key prompts along the bottom: a keycap with the key, then the action it performs.
    const KeyPrompt prompts[8] = {
        KeyPrompt{"W A S D", "MOVE"},
        KeyPrompt{"LMB", "SHOOT"},
        KeyPrompt{"Q", "BREACH ×" + std::to_string(breachChargesLeft)},
        KeyPrompt{"E", "HARD BREACH ×" + std::to_string(hardBreachChargesLeft)},
        KeyPrompt{"F", "DRONE"},
        KeyPrompt{"TAB", "ALGORITHMS"},
        KeyPrompt{"M", "MATH"},
        KeyPrompt{"R", "RESTART"},
    };
    unsigned int promptCharacterSize = static_cast<unsigned int>(unit * 2.f);
    float keycapPadding = unit * 0.9f;
    float keycapHeight = unit * 3.4f;
    float gapAfterKeycap = unit * 0.9f;
    float gapBetweenPrompts = unit * 2.4f;

    // First measure every prompt, so the whole row can be centered.
    float rowWidth = 0.f;
    for (const KeyPrompt& prompt : prompts)
    {
        sf::Text keyText(font, fromUtf8(prompt.key), promptCharacterSize);
        sf::Text actionText(font, fromUtf8(prompt.action), promptCharacterSize);
        keyText.setFillColor(Palette::HudText);
        actionText.setFillColor(Palette::HudMutedText);
        keyTexts.push_back(keyText);
        actionTexts.push_back(actionText);

        float keycapWidth = keyText.getLocalBounds().size.x + keycapPadding * 2.f;
        rowWidth += keycapWidth + gapAfterKeycap + actionText.getLocalBounds().size.x;
    }
    rowWidth += gapBetweenPrompts * static_cast<float>(keyTexts.size() - 1);

    // Then place them left to right.
    float rowMiddle = windowSize.y - unit * 4.f;
    float cursor = (windowSize.x - rowWidth) / 2.f;
    for (std::size_t promptIndex = 0; promptIndex < keyTexts.size(); ++promptIndex)
    {
        float keycapWidth = keyTexts[promptIndex].getLocalBounds().size.x + keycapPadding * 2.f;
        sf::RectangleShape keycap(sf::Vector2f(keycapWidth, keycapHeight));
        keycap.setPosition(sf::Vector2f(cursor, rowMiddle - keycapHeight / 2.f));
        keycap.setFillColor(Palette::KeycapFill);
        keycap.setOutlineColor(Palette::KeycapBorder);
        keycap.setOutlineThickness(unit * 0.15f);
        keycaps.push_back(keycap);
        centerText(keyTexts[promptIndex], sf::Vector2f(cursor + keycapWidth / 2.f, rowMiddle));
        cursor += keycapWidth + gapAfterKeycap;

        sf::Text& actionText = actionTexts[promptIndex];
        sf::FloatRect actionBounds = actionText.getLocalBounds();
        actionText.setOrigin(sf::Vector2f(actionBounds.position.x, actionBounds.position.y + actionBounds.size.y / 2.f));
        actionText.setPosition(sf::Vector2f(cursor, rowMiddle));
        cursor += actionBounds.size.x + gapBetweenPrompts;
    }
}

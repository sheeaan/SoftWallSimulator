#include "presentation/HeadsUpDisplay.hpp"

#include "presentation/Fonts.hpp"
#include "presentation/Palette.hpp"

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/View.hpp>

#include <string>

namespace
{
    // Centers a text on a point, using the text's own bounds.
    void centerText(sf::Text& text, sf::Vector2f center)
    {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.position + bounds.size / 2.f);
        text.setPosition(center);
    }

    struct KeyPrompt
    {
        std::string key;
        std::string action;
    };
}

HeadsUpDisplay::HeadsUpDisplay(sf::Vector2u windowSize)
    : font(loadFont("BarlowCondensed-SemiBold.ttf"))
    , unit(static_cast<float>(windowSize.y) / 100.f)
    , attackerText(font, "ATTACKER", static_cast<unsigned int>(unit * 2.4f))
    , statusText(font, "DEFENDER HIDDEN", static_cast<unsigned int>(unit * 2.4f))
    , defenderText(font, "DEFENDER", static_cast<unsigned int>(unit * 2.4f))
{
    // The banner: blue attacker block, dark status block, orange defender block, centered at the top.
    float windowWidth = static_cast<float>(windowSize.x);
    float bannerHeight = unit * 4.4f;
    float teamWidth = unit * 15.f;
    float statusWidth = unit * 26.f;
    float bannerLeft = (windowWidth - (teamWidth * 2.f + statusWidth)) / 2.f;
    float bannerTop = unit * 1.6f;

    attackerBlock.setSize(sf::Vector2f(teamWidth, bannerHeight));
    attackerBlock.setPosition(sf::Vector2f(bannerLeft, bannerTop));
    attackerBlock.setFillColor(Palette::AttackerBlue);
    statusBlock.setSize(sf::Vector2f(statusWidth, bannerHeight));
    statusBlock.setPosition(sf::Vector2f(bannerLeft + teamWidth, bannerTop));
    statusBlock.setFillColor(Palette::HudBackground);
    defenderBlock.setSize(sf::Vector2f(teamWidth, bannerHeight));
    defenderBlock.setPosition(sf::Vector2f(bannerLeft + teamWidth + statusWidth, bannerTop));
    defenderBlock.setFillColor(Palette::DefenderOrange);

    float bannerMiddle = bannerTop + bannerHeight / 2.f;
    attackerText.setFillColor(Palette::HudText);
    centerText(attackerText, sf::Vector2f(bannerLeft + teamWidth / 2.f, bannerMiddle));
    statusText.setFillColor(Palette::HudMutedText);
    centerText(statusText, sf::Vector2f(bannerLeft + teamWidth + statusWidth / 2.f, bannerMiddle));
    defenderText.setFillColor(Palette::HudText);
    centerText(defenderText, sf::Vector2f(bannerLeft + teamWidth * 1.5f + statusWidth, bannerMiddle));

    // Key prompts along the bottom: a keycap with the key, then the action it performs.
    const KeyPrompt prompts[4] = {
        KeyPrompt{"W A S D", "MOVE"},
        KeyPrompt{"LMB", "SHOOT WALL"},
        KeyPrompt{"TAB", "ALGORITHM VIEW"},
        KeyPrompt{"M", "MATH VIEW"},
    };
    unsigned int promptCharacterSize = static_cast<unsigned int>(unit * 2.f);
    float keycapPadding = unit * 0.9f;
    float keycapHeight = unit * 3.4f;
    float gapAfterKeycap = unit * 0.9f;
    float gapBetweenPrompts = unit * 3.f;

    // First measure every prompt, so the whole row can be centered.
    float rowWidth = 0.f;
    for (const KeyPrompt& prompt : prompts)
    {
        sf::Text keyText(font, prompt.key, promptCharacterSize);
        sf::Text actionText(font, prompt.action, promptCharacterSize);
        keyText.setFillColor(Palette::HudText);
        actionText.setFillColor(Palette::HudMutedText);
        keyTexts.push_back(keyText);
        actionTexts.push_back(actionText);

        float keycapWidth = keyText.getLocalBounds().size.x + keycapPadding * 2.f;
        rowWidth += keycapWidth + gapAfterKeycap + actionText.getLocalBounds().size.x;
    }
    rowWidth += gapBetweenPrompts * static_cast<float>(keyTexts.size() - 1);

    // Then place them left to right.
    float rowMiddle = static_cast<float>(windowSize.y) - unit * 4.f;
    float cursor = (windowWidth - rowWidth) / 2.f;
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

void HeadsUpDisplay::setEnemySpotted(bool spotted)
{
    if (spotted == enemySpotted)
    {
        return;
    }
    enemySpotted = spotted;

    sf::Vector2f statusCenter = statusText.getPosition();
    if (spotted)
    {
        statusText.setString("DEFENDER SPOTTED");
        statusText.setFillColor(Palette::DefenderOrange);
    }
    else
    {
        statusText.setString("DEFENDER HIDDEN");
        statusText.setFillColor(Palette::HudMutedText);
    }
    centerText(statusText, statusCenter);
}

void HeadsUpDisplay::draw(sf::RenderTarget& target) const
{
    // Laid out in pixels, so it draws with the window's default view.
    sf::View previousView = target.getView();
    target.setView(target.getDefaultView());

    target.draw(attackerBlock);
    target.draw(statusBlock);
    target.draw(defenderBlock);
    target.draw(attackerText);
    target.draw(statusText);
    target.draw(defenderText);

    for (std::size_t promptIndex = 0; promptIndex < keycaps.size(); ++promptIndex)
    {
        target.draw(keycaps[promptIndex]);
        target.draw(keyTexts[promptIndex]);
        target.draw(actionTexts[promptIndex]);
    }

    target.setView(previousView);
}

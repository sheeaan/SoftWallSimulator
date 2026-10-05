#include "presentation/StatisticsPanel.hpp"

#include "presentation/Fonts.hpp"
#include "presentation/Palette.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

namespace
{
    constexpr float RefreshSeconds = 0.25f;
    constexpr int LabelWidth = 14; // Characters. The font is monospaced, so columns line up.
    constexpr int ValueWidth = 12;
}

StatisticsPanel::StatisticsPanel(sf::Vector2u windowSize)
    : font(loadFont("JetBrainsMono-Regular.ttf"))
    , titleText(font, "SOFT WALL SIMULATOR", windowSize.y / 46)
    , bodyText(font, "", windowSize.y / 58)
    , padding(static_cast<float>(windowSize.y) / 60.f)
{
    titleText.setFillColor(Palette::PanelTitle);
    titleText.setLetterSpacing(1.5f);
    bodyText.setFillColor(Palette::PanelText);
    background.setFillColor(Palette::PanelBackground);
    accentBar.setFillColor(Palette::AttackerBlue);
}

void StatisticsPanel::update(float seconds, const World& world)
{
    framesSinceRefresh += 1;
    secondsSinceRefresh += seconds;
    if (secondsSinceRefresh < RefreshSeconds)
    {
        return;
    }

    float framesPerSecond = static_cast<float>(framesSinceRefresh) / secondsSinceRefresh;
    float frameMilliseconds = 1000.f * secondsSinceRefresh / static_cast<float>(framesSinceRefresh);
    framesSinceRefresh = 0;
    secondsSinceRefresh = 0.f;

    const SimulationStatistics& statistics = world.getStatistics();
    std::size_t nodesExpanded = world.getPathfinder().getExpandedCells().size();

    // Count only the cells still ahead of the enemy, not the ones it has already walked.
    const Enemy& enemy = world.getEnemy();
    std::size_t cellsAhead = enemy.getPath().size() - enemy.getNextWaypointIndex();
    std::string routeText = std::to_string(cellsAhead) + " cells";
    if (enemy.getPath().empty())
    {
        routeText = "no route";
    }
    std::string enemyText = "hidden";
    if (world.canPlayerSeeEnemy())
    {
        enemyText = "IN SIGHT";
    }
    std::string defenderStateText = "hunting";
    if (world.getDefenderState() == DefenderState::PREPARING)
    {
        defenderStateText = "reinforcing";
    }
    else if (world.getDefenderState() == DefenderState::GUARDING)
    {
        defenderStateText = "guarding the site";
    }

    std::ostringstream lines;
    lines << std::left << std::fixed;
    lines << std::setw(LabelWidth) << "FPS" << std::setw(ValueWidth) << std::lround(framesPerSecond)
          << std::setprecision(1) << frameMilliseconds << " ms per frame\n";
    lines << std::setw(LabelWidth) << "Vision (DDA)" << std::setw(ValueWidth) << (std::to_string(Raycaster::RayCount) + " rays")
          << std::setprecision(3) << statistics.visibilityMilliseconds << " ms\n";
    lines << std::setw(LabelWidth) << "Path (A*)" << std::setw(ValueWidth) << (std::to_string(nodesExpanded) + " nodes")
          << std::setprecision(3) << statistics.pathfindingMilliseconds << " ms\n";
    lines << std::setw(LabelWidth) << "Enemy route" << routeText << '\n';
    lines << std::setw(LabelWidth) << "Re-plans" << statistics.pathRecalculations << '\n';
    lines << std::setw(LabelWidth) << "Walls broken" << statistics.wallsDestroyed << '\n';
    lines << std::setw(LabelWidth) << "Defender" << defenderStateText << '\n';
    lines << std::setw(LabelWidth) << "Enemy" << enemyText;
    bodyText.setString(lines.str());

    // Lay the panel out around the text: the title, the body under it, padding all round.
    titleText.setPosition(sf::Vector2f(padding * 2.f, padding * 1.6f));
    sf::FloatRect titleBounds = titleText.getGlobalBounds();
    bodyText.setPosition(sf::Vector2f(padding * 2.f, titleBounds.position.y + titleBounds.size.y + padding));
    sf::FloatRect bodyBounds = bodyText.getGlobalBounds();

    float right = std::max(titleBounds.position.x + titleBounds.size.x, bodyBounds.position.x + bodyBounds.size.x) + padding;
    float bottom = bodyBounds.position.y + bodyBounds.size.y + padding;
    sf::Vector2f panelSize(right - padding, bottom - padding);
    background.setPosition(sf::Vector2f(padding, padding));
    background.setSize(panelSize);
    accentBar.setPosition(sf::Vector2f(padding, padding));
    accentBar.setSize(sf::Vector2f(padding * 0.25f, panelSize.y));
}

void StatisticsPanel::draw(sf::RenderTarget& target) const
{
    // Nothing to show before the first refresh, a quarter second in.
    if (bodyText.getString().isEmpty())
    {
        return;
    }

    // The panel is laid out in pixels, so it draws with the window's default view.
    sf::View previousView = target.getView();
    target.setView(target.getDefaultView());
    target.draw(background);
    target.draw(accentBar);
    target.draw(titleText);
    target.draw(bodyText);
    target.setView(previousView);
}

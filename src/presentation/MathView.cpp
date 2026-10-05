#include "presentation/MathView.hpp"

#include "presentation/Fonts.hpp"
#include "presentation/Palette.hpp"
#include "presentation/VertexQuads.hpp"
#include "simulation/Pathfinder.hpp"

#include <SFML/System/String.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>

namespace
{
    constexpr std::size_t MaximumTableSteps = 14;
    constexpr float TraceLineWidth = 0.05f;   // Tiles.
    constexpr float CellOutlineWidth = 0.04f; // Tiles.
    constexpr float CrossingDotSize = 0.16f;  // Tiles.
    constexpr float HitMarkerSize = 0.3f;     // Tiles.
    constexpr float HoverOutlineWidth = 0.07f;
    constexpr float ArrowLineWidth = 0.035f;
    constexpr float ArrowHeadLength = 0.13f;
    constexpr float ArrowHeadHalfWidth = 0.08f;
    constexpr int LensRadius = 4;               // Cells around the cursor whose A* labels show at full strength.
    constexpr std::uint8_t FadedLabelAlpha = 60; // Labels farther out stay visible but quiet.

    sf::Color withAlpha(sf::Color color, std::uint8_t alpha)
    {
        color.a = alpha;
        return color;
    }

    // The math symbols in these strings (×, −, Δ, →) are UTF-8 in the source. sf::Text wants UTF-32.
    sf::String fromUtf8(const std::string& text)
    {
        return sf::String::fromUtf8(text.begin(), text.end());
    }

    std::string formatCell(GridCell cell)
    {
        return "(" + std::to_string(cell.column) + ", " + std::to_string(cell.row) + ")";
    }

    std::string tileTypeName(TileType type)
    {
        switch (type)
        {
        case TileType::EMPTY:
            return "EMPTY";
        case TileType::SOFT_WALL:
            return "SOFT_WALL";
        case TileType::HARD_WALL:
            return "HARD_WALL";
        case TileType::REINFORCED_WALL:
            return "REINFORCED_WALL";
        case TileType::BARRICADE:
            return "BARRICADE";
        }
        return "UNKNOWN";
    }

    // The index of a neighboring cell, or "outside" at the edge of the map.
    std::string neighborIndexText(const Grid& grid, GridCell neighbor)
    {
        if (!grid.contains(neighbor))
        {
            return "outside";
        }
        return std::to_string(grid.indexOf(neighbor));
    }

    // Lays out a panel: the text at a corner, with a background padded around its bounds.
    void placePanel(sf::Text& text, sf::RectangleShape& background, sf::Vector2f topLeft, float padding)
    {
        text.setPosition(topLeft + sf::Vector2f(padding, padding));
        sf::FloatRect bounds = text.getGlobalBounds();
        background.setPosition(topLeft);
        background.setSize(sf::Vector2f(bounds.position.x + bounds.size.x + padding - topLeft.x,
                                        bounds.position.y + bounds.size.y + padding - topLeft.y));
    }
}

MathView::MathView(sf::Vector2u windowPixelSize, const Grid& grid)
    : font(loadFont("JetBrainsMono-Regular.ttf"))
    , windowSize(windowPixelSize)
    , unit(static_cast<float>(windowPixelSize.y) / 100.f)
    , pixelsPerTile(static_cast<float>(windowPixelSize.x) / static_cast<float>(grid.getColumnCount()))
    , axisLabels(font, static_cast<unsigned int>(pixelsPerTile * 0.34f), 1.f / pixelsPerTile)
    , costLabels(font, static_cast<unsigned int>(pixelsPerTile * 0.25f), 1.f / pixelsPerTile)
    , totalCostLabels(font, static_cast<unsigned int>(pixelsPerTile * 0.36f), 1.f / pixelsPerTile)
    , stepLabels(font, static_cast<unsigned int>(pixelsPerTile * 0.3f), 1.f / pixelsPerTile)
    , parentArrows(sf::PrimitiveType::Triangles)
    , traceShapes(sf::PrimitiveType::Triangles)
    , hoverOutline(sf::PrimitiveType::Triangles)
    , traceTable(font, "", static_cast<unsigned int>(unit * 1.5f))
    , inspectorText(font, "", static_cast<unsigned int>(unit * 1.5f))
{
    // Column numbers run along the top wall and row numbers down the left wall, so every cell has an address.
    for (int column = 0; column < grid.getColumnCount(); ++column)
    {
        axisLabels.append(std::to_string(column), sf::Vector2f(static_cast<float>(column) + 0.5f, 0.5f),
                          TextAlignment::CENTER, Palette::AxisLabel);
    }
    for (int row = 1; row < grid.getRowCount(); ++row)
    {
        axisLabels.append(std::to_string(row), sf::Vector2f(0.5f, static_cast<float>(row) + 0.5f),
                          TextAlignment::CENTER, Palette::AxisLabel);
    }

    traceTable.setFillColor(Palette::MathPanelText);
    inspectorText.setFillColor(Palette::MathPanelText);
    traceTableBackground.setFillColor(Palette::MathPanelBackground);
    inspectorBackground.setFillColor(Palette::MathPanelBackground);
}

void MathView::toggle()
{
    visible = !visible;
    labeledSearchNumber = -1; // Rebuild the A* labels the next time the view shows.
}

bool MathView::isVisible() const
{
    return visible;
}

void MathView::update(const World& world, sf::Vector2f cursorWorldPosition)
{
    if (!visible)
    {
        return;
    }

    // A* only searches when the player changes cell or a wall breaks, and the lens only moves
    // when the cursor changes cell, so the labels are rebuilt rarely.
    GridCell hoveredCell = cellContaining(cursorWorldPosition);
    bool searchChanged = world.getStatistics().pathRecalculations != labeledSearchNumber;
    if (searchChanged || hoveredCell != labeledHoverCell)
    {
        rebuildSearchLabels(world, hoveredCell);
        labeledSearchNumber = world.getStatistics().pathRecalculations;
        labeledHoverCell = hoveredCell;
    }
    rebuildRayTrace(world, cursorWorldPosition);
    rebuildCellInspector(world, hoveredCell);
}

void MathView::draw(sf::RenderTarget& window, const sf::View& cameraView) const
{
    if (!visible)
    {
        return;
    }

    // Labels and shapes live in world units, so they follow the camera, shake included.
    sf::View previousView = window.getView();
    window.setView(cameraView);
    axisLabels.draw(window);
    window.draw(parentArrows);
    costLabels.draw(window);
    totalCostLabels.draw(window);
    window.draw(traceShapes);
    stepLabels.draw(window);
    window.draw(hoverOutline);

    // The panels are laid out in pixels.
    window.setView(window.getDefaultView());
    window.draw(traceTableBackground);
    window.draw(traceTable);
    if (inspectorVisible)
    {
        window.draw(inspectorBackground);
        window.draw(inspectorText);
    }
    window.setView(previousView);
}

void MathView::rebuildSearchLabels(const World& world, GridCell hoveredCell)
{
    // Every cell the last search expanded gets an A* label: g in the top-left corner, f = g + h in the middle,
    // and h in the bottom-right corner. g is the cost from the enemy so far; h is the octile estimate to the player.
    // Labels near the cursor are bright; the rest fade, so the numbers under the cursor stay readable.
    const Grid& grid = world.getGrid();
    const Pathfinder& pathfinder = world.getPathfinder();
    costLabels.clear();
    totalCostLabels.clear();
    parentArrows.clear();

    for (std::size_t cellIndex : pathfinder.getExpandedCells())
    {
        GridCell cell = grid.cellAt(cellIndex);
        int costSoFar = pathfinder.getCostFromStart(cellIndex);
        int estimate = Pathfinder::estimateCost(cell, pathfinder.getGoal());

        int cellsFromCursor = std::max(std::abs(cell.column - hoveredCell.column), std::abs(cell.row - hoveredCell.row));
        sf::Color costSoFarColor = Palette::CostSoFar;
        sf::Color estimateColor = Palette::CostEstimate;
        sf::Color totalColor = Palette::TotalCost;
        if (cellsFromCursor > LensRadius)
        {
            costSoFarColor = withAlpha(costSoFarColor, FadedLabelAlpha);
            estimateColor = withAlpha(estimateColor, FadedLabelAlpha);
            totalColor = withAlpha(totalColor, FadedLabelAlpha);
        }

        float left = static_cast<float>(cell.column);
        float top = static_cast<float>(cell.row);
        costLabels.append(std::to_string(costSoFar), sf::Vector2f(left + 0.07f, top + 0.2f), TextAlignment::LEFT,
                          costSoFarColor);
        totalCostLabels.append(std::to_string(costSoFar + estimate), sf::Vector2f(left + 0.5f, top + 0.5f),
                               TextAlignment::CENTER, totalColor);
        costLabels.append(std::to_string(estimate), sf::Vector2f(left + 0.93f, top + 0.8f), TextAlignment::RIGHT,
                          estimateColor);

        // An arrow on the border with the cell this one was reached from. Following the arrows walks back to the enemy.
        std::optional<std::size_t> parentIndex = pathfinder.getCameFrom(cellIndex);
        if (!parentIndex.has_value())
        {
            continue;
        }
        sf::Vector2f center = cellCenter(cell);
        sf::Vector2f toParent = cellCenter(grid.cellAt(parentIndex.value())) - center;
        sf::Vector2f direction = toParent.normalized();
        sf::Vector2f arrowStart = center + toParent * 0.3f;
        sf::Vector2f arrowTip = center + toParent * 0.7f;
        sf::Vector2f headBase = arrowTip - direction * ArrowHeadLength;
        sf::Vector2f headSpread = direction.perpendicular() * ArrowHeadHalfWidth;
        appendLine(parentArrows, arrowStart, headBase, ArrowLineWidth, Palette::ParentArrow);
        parentArrows.append(sf::Vertex{arrowTip, Palette::ParentArrow});
        parentArrows.append(sf::Vertex{headBase + headSpread, Palette::ParentArrow});
        parentArrows.append(sf::Vertex{headBase - headSpread, Palette::ParentArrow});
    }
}

void MathView::rebuildRayTrace(const World& world, sf::Vector2f cursorWorldPosition)
{
    RayTrace trace = world.traceRayFromPlayer(cursorWorldPosition);
    traceShapes.clear();
    stepLabels.clear();

    // The ray itself, from the player to where the DDA stopped it.
    appendLine(traceShapes, trace.origin, trace.hit.point, TraceLineWidth, Palette::TraceRay);

    // Each cell the ray entered, outlined in the color of the boundary it crossed to get there,
    // with its step number and a dot where it crossed.
    sf::Vector2f halfDot(CrossingDotSize / 2.f, CrossingDotSize / 2.f);
    for (std::size_t stepIndex = 0; stepIndex < trace.steps.size(); ++stepIndex)
    {
        const DdaStep& step = trace.steps[stepIndex];
        sf::Color axisColor = Palette::RowCrossing;
        if (step.crossedColumn)
        {
            axisColor = Palette::ColumnCrossing;
        }
        sf::Vector2f cellTopLeft(static_cast<float>(step.cell.column), static_cast<float>(step.cell.row));
        appendOutline(traceShapes, cellTopLeft, cellTopLeft + sf::Vector2f(1.f, 1.f), CellOutlineWidth, axisColor);
        sf::Vector2f crossingPoint = trace.origin + trace.direction * step.crossingDistance;
        appendQuad(traceShapes, crossingPoint - halfDot, crossingPoint + halfDot, axisColor);
        stepLabels.append(std::to_string(stepIndex + 1), cellTopLeft + sf::Vector2f(0.2f, 0.24f), TextAlignment::CENTER,
                          axisColor);
    }
    sf::Vector2f halfMarker(HitMarkerSize / 2.f, HitMarkerSize / 2.f);
    appendOutline(traceShapes, trace.hit.point - halfMarker, trace.hit.point + halfMarker, CellOutlineWidth, Palette::TraceRay);

    // The same numbers as a table, in the notation of the DDA paper.
    std::ostringstream table;
    table << std::fixed << std::setprecision(3);
    table << "DDA   ray from the player toward the cursor\n";
    table << "origin (" << trace.origin.x << ", " << trace.origin.y << ")   direction (" << trace.direction.x << ", "
          << trace.direction.y << ")\n";
    table << "tDeltaX = 1 / |dx| = " << trace.distancePerColumn << "   tDeltaY = 1 / |dy| = " << trace.distancePerRow << "\n";
    table << "tMax = distance to the next boundary. Each step crosses the nearer one.\n\n";
    table << "step  enters       tMaxX     tMaxY    crosses       t\n";
    std::size_t shownSteps = std::min(trace.steps.size(), MaximumTableSteps);
    for (std::size_t stepIndex = 0; stepIndex < shownSteps; ++stepIndex)
    {
        const DdaStep& step = trace.steps[stepIndex];
        std::string crossed = "row";
        if (step.crossedColumn)
        {
            crossed = "column";
        }
        table << std::right << std::setw(4) << stepIndex + 1 << "  " << std::left << std::setw(10) << formatCell(step.cell)
              << std::right << std::setw(9) << step.distanceToNextColumn << std::setw(10) << step.distanceToNextRow << "    "
              << std::left << std::setw(8) << crossed << std::right << std::setw(9) << step.crossingDistance << "\n";
    }
    if (trace.steps.size() > shownSteps)
    {
        table << "      … and " << trace.steps.size() - shownSteps << " more steps\n";
    }
    table << "\nstops in " << formatCell(trace.hit.cell) << " at t = " << trace.hit.distance << ", point ("
          << trace.hit.point.x << ", " << trace.hit.point.y << ")";
    traceTable.setString(fromUtf8(table.str()));

    float padding = unit * 1.2f;
    sf::FloatRect tableBounds = traceTable.getLocalBounds();
    float tableWidth = tableBounds.position.x + tableBounds.size.x + padding * 2.f;
    placePanel(traceTable, traceTableBackground, sf::Vector2f(windowSize.x - tableWidth - unit * 1.6f, unit * 8.f), padding);
}

void MathView::rebuildCellInspector(const World& world, GridCell hoveredCell)
{
    const Grid& grid = world.getGrid();
    hoverOutline.clear();
    inspectorVisible = grid.contains(hoveredCell);
    if (!inspectorVisible)
    {
        return;
    }
    sf::Vector2f topLeft(static_cast<float>(hoveredCell.column), static_cast<float>(hoveredCell.row));
    appendOutline(hoverOutline, topLeft, topLeft + sf::Vector2f(1.f, 1.f), HoverOutlineWidth, Palette::HoverOutline);

    const Tile& tile = grid.getTile(hoveredCell);
    std::size_t index = grid.indexOf(hoveredCell);
    int columns = grid.getColumnCount();

    // The same offset measured from real addresses: the vector is one contiguous block, so pointers subtract.
    std::ptrdiff_t tilesFromStart = &tile - &grid.getTile(GridCell{0, 0});

    std::ostringstream lines;
    lines << "CELL " << formatCell(hoveredCell) << "   " << tileTypeName(tile.type);
    if (tile.type == TileType::SOFT_WALL)
    {
        lines << "   health " << static_cast<int>(tile.health) << " / " << static_cast<int>(Grid::SoftWallMaxHealth);
    }
    lines << "\n";
    lines << "index  = row × " << columns << " + column = " << hoveredCell.row << " × " << columns << " + "
          << hoveredCell.column << " = " << index << "\n";
    lines << "offset = index × sizeof(Tile) = " << index << " × " << sizeof(Tile) << " = " << index * sizeof(Tile)
          << " bytes\n";
    lines << "measured: &tile − &tiles[0] = " << static_cast<std::size_t>(tilesFromStart) * sizeof(Tile) << " bytes\n";
    lines << "neighbors: left " << neighborIndexText(grid, GridCell{hoveredCell.column - 1, hoveredCell.row})
          << "  right " << neighborIndexText(grid, GridCell{hoveredCell.column + 1, hoveredCell.row}) << "  up "
          << neighborIndexText(grid, GridCell{hoveredCell.column, hoveredCell.row - 1}) << "  down "
          << neighborIndexText(grid, GridCell{hoveredCell.column, hoveredCell.row + 1}) << "\n";
    if (tile.type == TileType::SOFT_WALL)
    {
        lines << "panels, 1 = intact, top row first: ";
        for (int panelRow = 0; panelRow < Grid::PanelsPerSide; ++panelRow)
        {
            for (int panelColumn = 0; panelColumn < Grid::PanelsPerSide; ++panelColumn)
            {
                char panelDigit = '0';
                if (isPanelIntact(tile, panelColumn, panelRow))
                {
                    panelDigit = '1';
                }
                lines << panelDigit;
            }
            lines << ' ';
        }
        lines << "\n";
    }

    // A*: the hovered cell's numbers, with the octile estimate worked out.
    const Pathfinder& pathfinder = world.getPathfinder();
    GridCell goal = pathfinder.getGoal();
    lines << "\nA*   last search: enemy " << formatCell(pathfinder.getStart()) << " → player " << formatCell(goal) << "\n";
    if (!pathfinder.wasExpanded(index))
    {
        lines << "not expanded by the last search";
    }
    else
    {
        int costSoFar = pathfinder.getCostFromStart(index);
        int columnDistance = std::abs(hoveredCell.column - goal.column);
        int rowDistance = std::abs(hoveredCell.row - goal.row);
        int diagonalSteps = std::min(columnDistance, rowDistance);
        int straightSteps = std::max(columnDistance, rowDistance) - diagonalSteps;
        int estimate = Pathfinder::estimateCost(hoveredCell, goal);

        lines << "g = " << costSoFar << "   cost from the enemy so far\n";
        lines << "h = " << Pathfinder::DiagonalStepCost << " × min(Δc, Δr) + " << Pathfinder::StraightStepCost
              << " × (max(Δc, Δr) − min(Δc, Δr))   with Δc = " << columnDistance << ", Δr = " << rowDistance << "\n";
        lines << "  = " << Pathfinder::DiagonalStepCost << " × " << diagonalSteps << " + " << Pathfinder::StraightStepCost
              << " × " << straightSteps << " = " << estimate << "\n";
        lines << "f = g + h = " << costSoFar << " + " << estimate << " = " << costSoFar + estimate << "\n";
        std::optional<std::size_t> parentIndex = pathfinder.getCameFrom(index);
        if (parentIndex.has_value())
        {
            lines << "parent " << formatCell(grid.cellAt(parentIndex.value()));
        }
        else
        {
            lines << "no parent: the search started here";
        }
    }
    inspectorText.setString(fromUtf8(lines.str()));

    float padding = unit * 1.2f;
    sf::FloatRect inspectorBounds = inspectorText.getLocalBounds();
    float inspectorWidth = inspectorBounds.position.x + inspectorBounds.size.x + padding * 2.f;
    float inspectorHeight = inspectorBounds.position.y + inspectorBounds.size.y + padding * 2.f;
    placePanel(inspectorText, inspectorBackground,
               sf::Vector2f(windowSize.x - inspectorWidth - unit * 1.6f, windowSize.y - inspectorHeight - unit * 8.f),
               padding);
}

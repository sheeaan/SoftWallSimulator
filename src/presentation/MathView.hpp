#pragma once

#include "presentation/TextBatch.hpp"
#include "simulation/World.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/View.hpp>

// The M key shows the math behind each system, using the numbers the code actually computed:
//  - Grid: row and column numbers along the map edges, and how the hovered cell maps into memory.
//  - DDA: the ray from the player toward the cursor, step by step, with its tMax and tDelta values.
//  - A*: g, h, and f on every cell the last search expanded, an arrow to each cell's parent,
//    and the octile heuristic worked out for the hovered cell.
// It draws straight to the window after the camera filter, so the numbers stay sharp.
class MathView
{
public:
    MathView(sf::Vector2u windowPixelSize, const Grid& grid);

    // The batches and texts point at the font member, so a copy would point at the original's font.
    MathView(const MathView&) = delete;
    MathView& operator=(const MathView&) = delete;

    void toggle();
    bool isVisible() const;

    // Forgets the cached A* labels, for when the world is replaced by a new round.
    void invalidate();

    // The cursor position is in world units.
    void update(const World& world, sf::Vector2f cursorWorldPosition);
    void draw(sf::RenderTarget& window, const sf::View& cameraView) const;

private:
    void rebuildSearchLabels(const World& world, GridCell hoveredCell);
    void rebuildRayTrace(const World& world, sf::Vector2f cursorWorldPosition);
    void rebuildCellInspector(const World& world, GridCell hoveredCell);

    sf::Font font;
    sf::Vector2f windowSize;
    float unit;               // One percent of the window height, in pixels.
    float pixelsPerTile;
    bool visible = false;
    int labeledSearchNumber = -1; // Which search the A* labels show, counted by re-plans.
    GridCell labeledHoverCell;    // Where the cursor was when the labels were built; labels near it are brighter.

    TextBatch axisLabels;
    TextBatch costLabels;      // g in the top-left corner and h in the bottom-right corner of each cell, small.
    TextBatch totalCostLabels; // f, larger, in the middle.
    TextBatch stepLabels;      // DDA step numbers.
    sf::VertexArray parentArrows;
    sf::VertexArray traceShapes;
    sf::VertexArray hoverOutline;

    sf::RectangleShape traceTableBackground;
    sf::Text traceTable;
    sf::RectangleShape inspectorBackground;
    sf::Text inspectorText;
    bool inspectorVisible = false;
};

#pragma once

#include <SFML/Graphics/Color.hpp>

// Every color lives here, so the look can be tuned in one place.
namespace Palette
{
    inline constexpr sf::Color Background{7, 9, 16};

    // Teams, as in Siege: attackers are blue, defenders are orange.
    inline constexpr sf::Color AttackerBlue{54, 150, 255};
    inline constexpr sf::Color DefenderOrange{255, 138, 36};

    // Floor and wall bodies are multiplied by the light map, so these are their fully lit colors.
    inline constexpr sf::Color FloorDark{58, 66, 92};
    inline constexpr sf::Color FloorLight{63, 72, 99};
    inline constexpr sf::Color Plaster{214, 206, 190};
    inline constexpr sf::Color Splinter{166, 112, 62};
    inline constexpr sf::Color SteelPlate{52, 60, 74};
    inline constexpr sf::Color SteelInnerPlate{66, 75, 91};
    inline constexpr sf::Color SteelBrace{38, 44, 56};
    inline constexpr sf::Color Rivet{132, 142, 160};
    inline constexpr sf::Color Concrete{92, 94, 100};
    inline constexpr sf::Color ConcreteSpeckle{74, 76, 82};
    inline constexpr sf::Color Plank{150, 104, 58};
    inline constexpr sf::Color PlankSeam{100, 68, 36};
    inline constexpr sf::Color SiteFloorTint{255, 138, 36, 38};

    // Edges skip the light map, so the outline of the map stays visible in the dark.
    inline constexpr sf::Color DrywallEdge{196, 190, 176};
    inline constexpr sf::Color SteelEdge{104, 118, 142};
    inline constexpr sf::Color ConcreteEdge{146, 150, 158};
    inline constexpr sf::Color BarricadeEdge{196, 146, 92};
    inline constexpr sf::Color SiteOutline{255, 138, 36, 210};

    // Destruction and gadgets.
    inline constexpr sf::Color TileFlash{255, 255, 255, 190};
    inline constexpr sf::Color PlasterDust{200, 194, 182, 210};
    inline constexpr sf::Color Spark{255, 196, 110};
    inline constexpr sf::Color MoltenSpark{255, 140, 30};
    inline constexpr sf::Color ReinforceSpark{200, 215, 235};
    inline constexpr sf::Color ChargeBody{40, 42, 48};
    inline constexpr sf::Color BreachChargeLight{255, 60, 60};
    inline constexpr sf::Color HardBreachGlow{255, 120, 30};
    inline constexpr sf::Color DroneBody{30, 34, 44};
    inline constexpr sf::Color DroneRing{54, 150, 255};
    inline constexpr sf::Color SpottedMarker{255, 138, 36};

    // The light map starts at the ambient level; each light adds to it.
    inline constexpr sf::Color AmbientLight{64, 70, 96};
    inline constexpr sf::Color Light{255, 240, 214};
    inline constexpr sf::Color DroneLight{150, 190, 255};

    // Glow alphas set the strength of their additive blend.
    inline constexpr sf::Color PlayerBody{160, 210, 255};
    inline constexpr sf::Color PlayerGlow{40, 130, 255, 170};
    inline constexpr sf::Color EnemyBody{255, 150, 52};
    inline constexpr sf::Color EnemyGlow{255, 110, 20, 170};

    // Statistics panel and heads-up display.
    inline constexpr sf::Color PanelBackground{8, 10, 20, 205};
    inline constexpr sf::Color PanelTitle{236, 240, 246};
    inline constexpr sf::Color PanelText{205, 214, 235};
    inline constexpr sf::Color HudBackground{12, 15, 22, 215};
    inline constexpr sf::Color HudText{238, 241, 246};
    inline constexpr sf::Color HudMutedText{150, 158, 172};
    inline constexpr sf::Color KeycapFill{24, 28, 38, 230};
    inline constexpr sf::Color KeycapBorder{190, 198, 212};
    inline constexpr sf::Color HudMessage{255, 214, 120};
    inline constexpr sf::Color ResultBackground{8, 10, 20, 225};

    // Math view. Column crossings and row crossings get their own colors, in the shapes and the table alike.
    inline constexpr sf::Color AxisLabel{150, 160, 180, 220};
    inline constexpr sf::Color CostSoFar{120, 200, 255};
    inline constexpr sf::Color CostEstimate{255, 170, 90};
    inline constexpr sf::Color TotalCost{240, 244, 250};
    inline constexpr sf::Color ParentArrow{200, 210, 230, 120};
    inline constexpr sf::Color TraceRay{255, 255, 255, 230};
    inline constexpr sf::Color ColumnCrossing{255, 214, 90};
    inline constexpr sf::Color RowCrossing{90, 220, 255};
    inline constexpr sf::Color HoverOutline{255, 255, 255};
    inline constexpr sf::Color MathPanelText{220, 228, 240};
    inline constexpr sf::Color MathPanelBackground{8, 10, 20, 240};

    // Algorithm overlay. The path belongs to the enemy, so it uses the defender's orange.
    inline constexpr sf::Color ExploredEarly{150, 90, 255, 60};
    inline constexpr sf::Color ExploredLate{40, 220, 200, 60};
    inline constexpr sf::Color Ray{255, 214, 120, 30};
    inline constexpr sf::Color VisibilityOutline{255, 214, 120, 190};
    inline constexpr sf::Color Path{255, 138, 36, 220};
    inline constexpr sf::Color PathFlash{255, 255, 255, 255};
}

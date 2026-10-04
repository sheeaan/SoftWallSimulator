# Soft Wall Simulator

A 2D destruction and vision sandbox in C++17 and SFML 3, built without an engine.
Break a soft wall and the world reacts in the same frame.
360 DDA rays re-light the map through the new hole.
An A\* hunter re-plans its route through it before the next frame is drawn.

![The player's light reaches through two broken walls and catches the enemy in the far opening](docs/gameplay.png)

## Highlights

- **Cache-friendly map.** The grid is one `std::vector<Tile>` in row-major order, 2 bytes per tile. There is no vector of vectors.
- **DDA raycasting.** Every frame casts 360 rays with Amanatides–Woo grid traversal. Each ray visits exactly the cells it crosses and stops at the first non-empty tile.
- **The visibility polygon is the light.** The ray hits form a triangle fan. It is drawn into a light map with a radial gradient, and the light map is multiplied over the scene.
- **A\* pathfinding.** Moves go in 8 directions with integer costs of 10 and 14 and never cut a wall's corner. The search uses the octile heuristic, a binary heap, and flat per-cell arrays that are reused across searches.
- **Instant re-planning.** `World::damageTileAt` re-plans the enemy's path inside the call that breaks the wall. A test proves it.
- **Game feel.** Soft walls show crack stages and flash when hit. Debris and sparks come from preallocated particle pools. Screen shake follows a trauma model.
- **Algorithm view.** Tab shows the 360 rays, the polygon outline, every cell A\* expanded colored by expansion order, and the live path.
- **Headless tests.** The simulation layer contains no rendering code, so its 10 tests run without a window.

On an Intel Core Ultra 5 226V (Release build), the panel reports about 0.03 ms for the 360-ray cast.
An A\* search takes about 0.2 ms when it expands 656 cells at startup, and about 0.02 ms once a broken wall opens a straight route.

![Algorithm view: the ray fan, the polygon outline, the cells A* expanded, and the enemy's path through both holes](docs/algorithm-view.png)

## Controls

| Input | Action |
|---|---|
| W A S D | Move |
| Left click | Hit the wall under the cursor. A soft wall breaks after 3 hits. A hard wall never breaks. |
| Tab | Toggle the algorithm view |

## Build and run

You need CMake 3.28 or newer, a C++17 compiler, and Git.
CMake downloads and builds SFML 3.0.2 during the first configure.

```
cmake -S . -B build
cmake --build build --config Release
build/bin/Release/SoftWallSimulator.exe
build/bin/Release/SoftWallTests.exe
```

Single-configuration generators such as Ninja put the programs in `build/bin/` instead.
The project was built and tested on Windows 11 with MSVC 19.29 (Visual Studio 2019 Build Tools).

## Architecture

The code has two layers.
`simulation/` holds the rules and links only `SFML::System`, for `sf::Vector2f`.
`presentation/` owns the window, input, and drawing, and reads the simulation through const references.

```
src/
  main.cpp                  Catches SimulatorError and prints its message.
  simulation/
    Grid                    Tile storage, bounds checks, damage.
    Level                   ASCII level parser and the 48 x 27 default map.
    Raycaster               DDA rays, the 360-ray visibility cast, line of sight.
    Pathfinder              A* with buffers reused across searches.
    Player                  WASD movement with box collision, one axis at a time.
    Enemy                   Walks its path at constant speed.
    World                   Owns all of the above and decides when the enemy re-plans.
    SimulatorErrors.hpp     SimulatorError and the exceptions derived from it.
  presentation/
    Game                    Window, loop, input mapping, screen shake, draw order.
    TileMapRenderer         Floor and walls in vertex arrays, cracks, hit flashes.
    LightingRenderer        Turns the visibility polygon into a light map.
    ParticleSystem          Fixed-capacity particle pool.
    AlgorithmOverlay        The Tab view.
    StatisticsPanel         Live numbers and controls.
tests/
  SimulationTests.cpp       Framework-free tests that print PASS or FAIL.
```

Each frame runs in this order:

1. Handle events. A click calls `World::damageTileAt`, which re-plans when a wall breaks.
2. Update the world: move the player, re-plan if the player entered a new cell, walk the enemy, cast 360 rays.
3. Draw the floor, the debris, and the wall bodies.
4. Multiply the light map over them.
5. Draw what must stay visible in the dark: wall outlines, flashes, the overlay, the enemy (when in sight), the player, sparks, and the panel.

## How it works

### The grid

Cell `(column, row)` lives at index `row * columnCount + column`.
Every loop over the map walks rows on the outside and columns on the inside, which matches the memory order.
Cells outside the map count as walls for vision, so every ray is guaranteed to stop.

### DDA raycasting

A ray tracks two distances: to the next column boundary and to the next row boundary.
Each step crosses whichever boundary is closer and enters the cell behind it.
A step is one comparison, one addition, and one tile lookup, with no trigonometry.
The 360 ray directions are computed once, when the `Raycaster` is constructed.

### Light from the visibility polygon

The 360 hit points, in angle order, form a triangle fan around the player.
Each vertex gets texture coordinates into a radial gradient centered on the player, so brightness falls off with true distance.
The fan is added on top of a dim ambient color in a render texture, and that light map is multiplied over the floor and walls.
Wall tiles that a ray hit are lit as brightly as the floor at that ray's end.

### A\*

The open set is a binary heap in a member `std::vector`, kept with `std::push_heap` and `std::pop_heap`.
Each cell can enter the heap more than once. Only its first, cheapest entry is expanded.
On equal estimates, the heap prefers the entry closer to the goal, which keeps the search heading forward.
A diagonal step is allowed only when both cells beside it are open, so the enemy never clips a corner.

### Destruction

A click maps from pixel to world through the same view the frame was drawn with, so it stays accurate during screen shake.
`std::floor`, not a cast, maps world to cell, so a click left of the map never lands in column 0.
A soft wall loses one health per hit and becomes empty at zero.
The world then re-plans the enemy's path before `damageTileAt` returns.

## Tests

```
Soft Wall Simulator - simulation tests

PASS  Grid stores cells in row-major order in one vector
PASS  A click position maps to the cell under it
PASS  Clicking a soft wall reduces its health by one
PASS  A soft wall becomes EMPTY at zero health
PASS  Vision casts exactly 360 rays
PASS  A ray stops at the first non-EMPTY tile
PASS  A* finds the shortest path around a wall
PASS  Destroying a wall recalculates the enemy path within the same call
PASS  End to end: breaking a soft wall opens a shortcut, and the enemy walks through it
PASS  End to end: breaking the wall between player and enemy gives line of sight

10 passed, 0 failed
```

The test program exits with code 1 if any test fails.

## Engineering choices

- **Warnings are errors.** Project targets build at `/W4 /permissive-` on MSVC and `-Wall -Wextra -Wpedantic` elsewhere. SFML is included as a system dependency, so its headers do not trip these warnings.
- **No manual memory management.** There is no `new` or `delete`. Containers and SFML objects own every resource.
- **Buffers are reused in hot paths.** Rays, the light map, particles, and the overlay keep their buffers between frames. A\* reuses its per-cell arrays, so only the returned path allocates.
- **Errors are specific.** `GridBoundsError`, `LevelFormatError`, and `ResourceError` derive from `SimulatorError`, and each carries a message that names the problem.
- **The code favors plain reading.** Types are written out instead of `auto`, loops are explicit, and names are full words.

## Credits

- [SFML 3.0.2](https://www.sfml-dev.org/) under the zlib license, downloaded at configure time.
- [JetBrains Mono](https://www.jetbrains.com/lp/mono/) under the SIL Open Font License 1.1. The font and its license are in `assets/fonts/`.

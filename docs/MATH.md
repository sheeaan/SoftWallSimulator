# The math behind Soft Wall Simulator

Each section derives one formula the code uses and names the function that implements it.
The in-game math view (press **M**) shows the same numbers live.
The test suite checks the worked examples below.

World positions are measured in tiles, so cell $(c, r)$ covers $c \le x < c + 1$ and $r \le y < r + 1$.
The map is $W \times H = 48 \times 27$ cells.

---

## 1. The grid: a 2D map in one array

`Grid::indexOf`, `Grid::cellAt`, `cellContaining` in `src/simulation/Grid.cpp`.

The tiles live in one `std::vector<Tile>`, row after row (row-major order):

$$
\text{index}(c, r) = r \cdot W + c
\qquad\qquad
c = \text{index} \bmod W, \quad r = \left\lfloor \frac{\text{index}}{W} \right\rfloor
$$

A `Tile` is 4 bytes: a 1-byte type, a 1-byte health, and a 2-byte panel mask.
So a cell's byte offset from the start of the array is

$$
\text{offset}(c, r) = 4 \cdot (r \cdot W + c).
$$

**Example.** Cell $(12, 7)$ has index $7 \cdot 48 + 12 = 348$ and starts at byte $348 \cdot 4 = 1392$.
The math view also measures this from real addresses, `&tile - &tiles[0]`, and gets the same number.

**Why it is fast.** The left and right neighbors are at $\text{index} \pm 1$, usually on the same 64-byte cache line.
The neighbors above and below are at $\text{index} \pm W$, 192 bytes away.
Every loop over the map runs rows on the outside and columns on the inside, so it reads memory front to back.

**Mouse to cell.** A click becomes a world position $p$, and then a cell:

$$
(c, r) = \left( \lfloor p_x \rfloor, \lfloor p_y \rfloor \right)
$$

The code uses `std::floor`, not a cast.
A cast rounds toward zero, which would put $p_x = -0.5$ in column 0 instead of column $-1$, outside the map.

---

## 2. DDA: walking a ray through the grid

`Raycaster::castRay` in `src/simulation/Raycaster.cpp`.
This is Amanatides and Woo, *A Fast Voxel Traversal Algorithm* (1987).

A ray starts at $o$ and points along the unit vector $d$.
Its points are $p(t) = o + t\,d$, where $t$ is the distance travelled, in tiles.

**tDelta.** Moving one whole column changes $x$ by 1, which takes $|d_x|\,t = 1$:

$$
\Delta t_x = \frac{1}{|d_x|} \qquad \Delta t_y = \frac{1}{|d_y|}
$$

In the code these are `distancePerColumn` and `distancePerRow`.
They are constant for the whole ray.

**tMax.** These values give the distance to the first column boundary and the first row boundary.
Starting in column $c = \lfloor o_x \rfloor$:

$$
t^{\max}_x =
\begin{cases}
(c + 1 - o_x)\,\Delta t_x & d_x > 0 \\
(o_x - c)\,\Delta t_x & d_x < 0 \\
\infty & d_x = 0
\end{cases}
$$

$t^{\max}_y$ uses the same formula with rows.
In the code these are `distanceToNextColumn` and `distanceToNextRow`.

**The loop.** Each step crosses whichever boundary is nearer and enters the cell behind it:

$$
\text{if } t^{\max}_x < t^{\max}_y:\quad t \leftarrow t^{\max}_x,\;\; t^{\max}_x \mathrel{+}= \Delta t_x,\;\; c \mathrel{+}= \operatorname{sign}(d_x)
$$

$$
\text{else}:\quad t \leftarrow t^{\max}_y,\;\; t^{\max}_y \mathrel{+}= \Delta t_y,\;\; r \mathrel{+}= \operatorname{sign}(d_y)
$$

The ray stops at the first cell that blocks vision, at the point $o + t\,d$ on that cell's face.
A step is one comparison, one addition, and one tile lookup, with no trigonometry.

**It always stops.** A ray crosses each of the $W + 1$ column lines and $H + 1$ row lines at most once.
Cells outside the map block vision.
So the loop ends within $W + H + 2$ steps.

**Worked example** (the test "The DDA trace records tMaxX, tMaxY, and the axis of every step").
The ray starts at $o = (0.5, 0.5)$ with $d = (2, 1)/\sqrt5 \approx (0.894, 0.447)$, and a wall stands at $(3, 1)$.
Then $\Delta t_x = \sqrt5/2 \approx 1.118$ and $\Delta t_y = \sqrt5 \approx 2.236$.
The first boundaries are $t^{\max}_x = 0.5 \cdot 1.118 = 0.559$ and $t^{\max}_y = 0.5 \cdot 2.236 = 1.118$.

| step | $t^{\max}_x$ | $t^{\max}_y$ | crosses | enters | $t$ |
|---|---|---|---|---|---|
| 1 | 0.559 | 1.118 | column | (1, 0) | 0.559 |
| 2 | 1.677 | 1.118 | row | (1, 1) | 1.118 |
| 3 | 1.677 | 3.354 | column | (2, 1) | 1.677 |
| 4 | 2.795 | 3.354 | column | (3, 1), the wall | 2.795 |

The ray hits the wall at $o + 2.795\,d = (3.0, 1.75)$, on its left face.

---

## 3. Bullet holes: the same DDA, one level down

`panelsAlongShot` in `Grid.cpp`, `Raycaster::findStopDistance` in `Raycaster.cpp`.

Each soft wall is a $4 \times 4$ grid of panels.
A 16-bit mask stores them, one bit per panel, row-major again:

$$
\text{bit}(c', r') = 2^{\,4r' + c'}
$$

**A shot drills a slot.** The shot travels from the player at $s$ to the clicked point $q$.
When $|q_x - s_x| \ge |q_y - s_y|$, the shot is mostly horizontal, and it clears the whole row of panels at the hit height:

$$
r' = \left\lfloor 4\,(q_y - r) \right\rfloor
$$

Otherwise it clears the column $c' = \lfloor 4\,(q_x - c) \rfloor$.
Each hit also takes one health.
At zero health the tile becomes `EMPTY`.

**Seeing through.** When a ray enters a soft wall with holes at distance $t_0$, the code runs the DDA again inside the tile, with panel-sized cells:

$$
\Delta t'_x = \frac{1/4}{|d_x|} \qquad \Delta t'_y = \frac{1/4}{|d_y|}
$$

The ray stops at the first intact panel.
If it leaves the tile first, it went through the holes, and the outer DDA continues.
The second DDA takes at most $2 \cdot 4 + 1$ steps.

**Only along the line of fire.** A horizontal slot is $1/4$ tile tall.
To stay inside it across the full tile width, a ray must climb less than $1/4$ over a run of 1, so $|d_y / d_x| < 1/4$.
It must also enter at the slot's height.
A player who steps sideways enters at another height and hits plaster.
The test "a bullet hole gives sight only along the line of fire" checks exactly this.

---

## 4. Light from the visibility polygon

`Raycaster::Raycaster`, `LightingRenderer::drawLightMap`.

The 360 ray directions are computed once:

$$
\theta_k = \frac{2\pi k}{360}, \qquad d_k = (\cos\theta_k, \sin\theta_k), \qquad k = 0, \dots, 359
$$

In angle order, the 360 hit points form a triangle fan around the player: the visibility polygon.

**Falloff.** Brightness at distance $\rho$ from the player, with light radius $R = 24$ tiles, is

$$
b(\rho) = 1 - \operatorname{smoothstep}\!\left(\frac{\rho}{R}\right), \qquad \operatorname{smoothstep}(x) = 3x^2 - 2x^3 \text{ for } 0 \le x \le 1,
$$

and $b = 0$ beyond $R$.
The curve is flat at both ends, so the light has a soft core and a soft edge.

**Exact distance from interpolation.** The falloff is baked into a $256 \times 256$ texture once.
Each fan vertex $p$ gets texture coordinates

$$
uv = 128 + 128 \cdot \frac{p - o}{R}.
$$

These coordinates are an affine function of position, and the GPU interpolates them linearly across each triangle.
So every pixel samples the texture at its own offset from the player, and the falloff follows true radial distance even inside long, thin triangles.

**Composition.** The light map starts at an ambient color $A$, then adds the fan and the lit wall faces.
The scene is multiplied by it:

$$
\text{pixel} = \text{scene} \times \min\!\big(1,\; A + b(\rho)\,L\big).
$$

---

## 5. A*: g, h, and f

`Pathfinder::findPath`, `Pathfinder::estimateCost` in `src/simulation/Pathfinder.cpp`.

The search runs from the enemy to the player.
For a cell $n$:

- $g(n)$ is the cheapest known cost from the enemy.
- $h(n)$ estimates the remaining cost to the player.
- $f(n) = g(n) + h(n)$ orders the open set, a binary heap.

**Costs.** A straight step costs 10 and a diagonal step costs 14.
That is $10\sqrt2 \approx 14.14$ rounded, so all arithmetic stays in integers.

**The octile heuristic.** With $\Delta c = |c_n - c_{goal}|$ and $\Delta r = |r_n - r_{goal}|$:

$$
h(n) = 14 \cdot \min(\Delta c, \Delta r) + 10 \cdot \big(\max(\Delta c, \Delta r) - \min(\Delta c, \Delta r)\big)
$$

On an open grid, this is the exact cheapest cost.
Take one diagonal step for each unit the axes share, then go straight for the rest.
A diagonal step costs 14, and two straight steps cost 20, so using diagonals is always cheaper.

**Admissible.** Walls only remove moves, so a real path never costs less than $h$.
A* therefore returns a cheapest path.

**Consistent.** One step changes $\Delta c$ and $\Delta r$ by at most 1 each, so $h$ changes by at most the step's cost:

$$
h(a) \le \operatorname{cost}(a, b) + h(b).
$$

Then the first time A* pops a cell, its $g$ is final.
That is why a cell can sit in the heap more than once: only its first, cheapest entry is expanded, and later copies are skipped.

**Ties.** On equal $f$, the heap pops the entry with the smaller $h$, the one closer to the goal.
This keeps the search heading forward and expands fewer cells.

**No corner cutting.** A diagonal step from $(c, r)$ to $(c + s_c, r + s_r)$ is allowed only when both $(c + s_c, r)$ and $(c, r + s_r)$ are open.
Otherwise the enemy would clip a wall's corner.

**Worked example** (the test "A* finds the shortest path around a wall").
The start is $(1, 2)$, the goal is $(5, 2)$, and a wall fills column 3 from row 1 to row 3.
Then $h(\text{start}) = 10 \cdot 4 = 40$.
Without corner cutting, the cheapest detour is two diagonal steps and four straight steps:

$$
2 \cdot 14 + 4 \cdot 10 = 68 \;\ge\; 40.
$$

**Along any found route**, each step adds exactly its own cost to $g$: 10 straight or 14 diagonal.
The test "along the enemy's route, each step adds its own cost to g" checks every step.

**Cost of a search.** Each pop and push on the heap costs $O(\log n)$.
The per-cell arrays for $g$, the parent, and the state are reused between searches, so after the first search only the returned path allocates.

---

## 6. Destruction and instant re-planning

`World::damageTileAt` in `src/simulation/World.cpp`.

A hit on a soft wall does three things: health goes down by one, the slot's bits are cleared from the panel mask, and at zero health the type becomes `EMPTY`.
A new `EMPTY` tile can only make paths shorter.
`damageTileAt` therefore re-runs A* before it returns, so the enemy's next frame already uses the new route.

The enemy also re-plans when the player enters a new cell.
Within one cell, the old path still ends at the player.

---

## 7. Movement, collision, and the camera

`Player::move`, `Enemy::followPath`, `Game::update`.

**Diagonal speed.** The input vector is normalized before scaling, so moving diagonally is not $\sqrt2$ times faster.

**Collision.** The player is a box with half-width $0.3$.
It moves one axis at a time, and an axis that would overlap a wall is skipped, so the player slides along walls.
The box is smaller than a tile, so it touches at most $2 \times 2$ cells.

**Constant enemy speed.** Each frame the enemy has $v\,\Delta t$ tiles to travel.
Reaching a waypoint carries the remainder on to the next one, so speed does not dip at corners.

**Screen shake.** Hits add *trauma* $T \in [0, 1]$, and trauma decays linearly over time.
The camera offset is

$$
\text{offset} = S_{\max} \cdot T^2 \cdot (u_1, u_2), \qquad u_i \sim \mathcal{U}(-1, 1).
$$

Squaring $T$ makes small hits barely move the camera, while a breaking wall kicks it hard.

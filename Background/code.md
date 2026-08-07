# GLUT 2D Platformer Game Scene - Code Documentation

## Overview

This is a complete GLUT/OpenGL 2D graphics application that renders a
pixel-art style platformer background, matching the reference artwork
(`WhatsApp Image 2026-08-07 at 21.22.43.jpeg`) in this folder, and then
**scrolls that whole scene left forever** - the same idea as the ground and
obstacles endlessly sliding by in the Chrome "dinosaur" game. The scene
includes:
- A gradient sky with layered clouds (soft washes, shadow puffs, cream
  cumulus banks, one crisp flat-bottomed cloud, and drifting sky particles),
  scrolling slower than the ground for a parallax depth cue
- A stepped brick-and-grass terrain: the main ground plus two platforms on
  the right ("ledge" and "tower") that lock together into one landmass
- Wispy, hand-drawn-looking grass blades along every grass rim, not a plain
  sine wave
- Scalloped two-tone bushes
- Three rivetted crates (one floating, one on the ground, one on the tower)
- A steadily increasing scroll speed, capped at a maximum, dino-game style

**Status**: Rewritten to match the reference image, then made to scroll
infinitely; builds clean with `make`.
**Executable**: `game_macos` (compiled for ARM64 on macOS)
**Platform**: macOS (with Linux compatibility via Makefile)

---

## Source Code Files

### 1. game.cpp - Main Application

```cpp
#ifdef __APPLE__
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
#else
    #include <GL/glut.h>
    #include <GL/gl.h>
#endif
```

#### The art-space canvas

Every shape in the file is authored in a fixed **735 x 412 "art space"** -
the same pixel dimensions as the reference JPEG - and converted to the
1200 x 700 GL canvas with four small helpers:

```cpp
static inline float IX(float x) { return (x + tileOriginX - currentScrollX) * SX; }
static inline float IY(float y) { return (ART_H - y) * SY; }   // y flips
static inline float IW(float w) { return w * SX; }
static inline float IH(float h) { return h * SY; }
```

`IY` flips the y-axis because the reference image (and every coordinate we
measured from it) has y=0 at the **top**, while OpenGL's ortho projection has
y=0 at the **bottom**. Because the whole scene is expressed in art space and
scaled, resizing the window never changes the composition - only its DPI.

#### Scrolling: `currentScrollX` / `tileOriginX`

`IX` has two extra terms beyond the plain art→canvas scale:

- `tileOriginX` - the world-space x of the *tile copy* currently being
  drawn (a multiple of `TILE_W`, see below)
- `currentScrollX` - how far the active layer has scrolled so far

Every drawing function in the file - bricks, grass, bushes, crates, clouds -
calls through `rectI`/`blobI`/`polyI`/`strokeI`/`roundRectI`, which all
funnel through `IX`/`IY`. That means **none of those functions know
scrolling exists**: `display()` sets `tileOriginX`/`currentScrollX` before
each group of draw calls, and everything downstream just lands in the right
place. `drawSky()` is the one exception - it paints a full-canvas gradient
quad directly in canvas space, since a flat gradient has nothing to scroll.

```cpp
static double scrollX = 0.0;   // ground-layer world offset, art units
static double speed   = 130.0; // current scroll speed, art units / second
```

`scrollX` only ever increases (there's no wraparound in the variable
itself - `double` has ample precision for any realistic runtime). What
*looks* like wraparound is `drawScrollingLayer()` picking a small, always-
valid range of tile copies to draw:

```cpp
template <typename Fn>
static void drawScrollingLayer(float layerScrollX, Fn drawTile) {
    currentScrollX = layerScrollX;
    int k0 = (int)floorf(layerScrollX / TILE_W) - 1;
    for (int k = k0; k <= k0 + 2; k++) {
        tileOriginX = k * TILE_W;
        drawTile();
    }
    tileOriginX = 0.0f;
}
```

`TILE_W` is `ART_W` (735) - the whole hand-authored scene is one tile. Three
side-by-side copies (`k0`, `k0+1`, `k0+2`) are always enough to cover the
735-wide viewport plus a one-tile margin on each side, so nothing pops in or
out at the edges as `scrollX` changes. `display()` calls this once for the
cloud layer (with `scrollX * CLOUD_PARALLAX`, so the sky drifts slower than
the ground) and once for everything else (terrain, platforms, crates,
bushes, at full `scrollX`) - which is also why the combined scene doesn't
look like an obviously short loop: the two layers are each individually
periodic, but at different periods, so they drift in and out of phase with
each other.

#### Core Drawing Primitives

```cpp
void drawFilledRect(float x, float y, float width, float height)   // GL_QUADS, canvas space
void drawRectOutline(float x, float y, float width, float height)  // GL_LINE_LOOP
void drawTriangle(...)                                              // GL_TRIANGLES
void drawCircle(float x, float y, float radius, int segments)       // GL_TRIANGLE_FAN
void drawEllipse(float cx, float cy, float rx, float ry, int segments)
```

Plus a set of **art-space wrappers** that everything else in the file is
built from:

- `rectI(x, y, w, h)` - rectangle given by its top-left corner, in art space
- `blobI(cx, cy, r)` - a circle in art space (renders as an ellipse on
  canvas, since `SX != SY`)
- `polyI(pts[], count)` - filled triangle-fan polygon from an art-space point list
- `strokeI(pts[], count, widthArt)` - a line strip with an art-space line width
- `roundRectI(x, y, w, h, r)` / `roundRectOutlineI(...)` - rounded rectangles,
  used for the crates and the pale sky "motes"
- `hash01(a, b)` - a tiny deterministic hash used everywhere jitter is
  needed (brick joints, grass blade height, tufts) so the scene is identical
  every frame without storing any random state

#### Scene Components

**Sky** (`drawSky()`)
- A single gradient quad from `SKY_BOTTOM` (#72EBFC) at y=0 to `SKY_TOP`
  (#41A4F7) at y=700, interpolated per-vertex by OpenGL.

**Clouds** (`drawClouds()`) - drawn as four layers, back to front:
1. `drawHazeClouds()` - pale opaque washes (`HAZE`) in the corners
2. `drawShadowClouds()` - muted blue-grey puffs (`CLOUD_GREY`) tucked behind
   the brighter masses
3. `drawCreamClouds()` - the big soft cream (`CLOUD_CREAM`) cumulus banks
   that read as the main cloud cover
4. `drawMainCloud()` - the single crisp white, flat-bottomed cloud in the
   middle of the sky
5. `drawSkyParticles()` - small flower blobs, diagonal dashes, and rounded
   "motes" scattered across the blue

All four cloud masses are built from `drawLobes()`: a handful of large,
closely-overlapping circles (`blobI`) plus a rectangle filling each lobe down
to a shared baseline. **They are drawn fully opaque, not alpha-blended** -
overlapping the same flat colour twice looks identical to once, so there are
no visible seams where circles cross. (An earlier alpha-blended version
produced a "bubbly" look with visible rings at every overlap - see git
history / the comment above `drawHazeClouds()`.)

**Terrain** (`drawBrickLayer()`, `drawPlatforms()`)

The ground and the two right-hand platforms are all instances of one
`Slab` struct - left/right edge, grass-surface y, brick-bottom y, grass
depth, seam depth, soil-cap depth, which edges to stroke, and the x
positions of the grass "fins" that stick up out of the rim:

```cpp
static const Slab GROUND = { 0,   740, 306, 412, 20, 41, 17, 0, MAIN_FINS,  14, 3 };
static const Slab LEDGE  = { 546, 623, 216, 306, 16, 30, 10, 1, LEDGE_FINS,  2, 11 };
static const Slab TOWER  = { 623, 740, 126, 306, 20, 38,  8, 1, TOWER_FINS,  3, 23 };
```

`LEDGE.x1 == TOWER.x0` exactly, and only `TOWER` strokes that shared edge -
so the two platforms read as one continuous stepped landmass instead of
leaving a stray outline floating inside the ledge's grass cap.

Each slab is drawn in two passes:
- `drawBrickFace(slab)` - courses of alternating-tone bricks with irregular,
  hashed perpend (vertical joint) positions, an accent brick dropped in
  occasionally, and a highlight strip along the top of every course
- `drawGrassCap(slab)` - the bright grass surface, two `drawBladeBand()`
  layers of wispy triangular blades (not a sine wave - see below), a wavy
  dark-soil seam, scattered "v" tufts, and the black rim stroke on top

```cpp
static void drawBladeBand(const Slab& s, float baseY, float botY, float tipY, int seed)
```
fills a solid band of turf and fringes its top edge with independently
hashed triangular blades - width, height and a small horizontal "lean" all
vary per blade - so the silhouette reads as grass rather than a regular
zig-zag.

**Bushes** (`drawBushes()`)

A single hand-placed cluster of 14 overlapping lobes (`BUSH_SHAPE`), each
tagged light/mid/dark. `drawBush()` draws the whole cluster's outline first
(fattened by 2 art px), then the three tone passes back-to-front so the dark
lobes always sit in front. `flip` mirrors the cluster horizontally and
`shade` pushes every lobe one tone darker, for the two bushes sitting in
shadow on the right.

**Crates** (`drawFloatingBlocks()`, `drawSmallBlocks()`)

`drawCrate(x, y, w, h, gloss)` draws the rivetted orange crate that appears
three times in the scene: outline → fill → pale top lip / warm floor strip
→ optional glossy crescent highlight (`gloss=1`, built from a thick
part-circle arc, not a straight bar) → four corner rivets. The crate on the
tower (`gloss=0`) instead gets two faint vertical panel lines, matching the
flatter look of that one in the reference art.

#### Rendering Pipeline

```cpp
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, CANVAS_W, 0, CANVAS_H, -1, 1);   // fixed virtual canvas
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    drawSky();   // one full-canvas gradient - never scrolls

    drawScrollingLayer((float)(scrollX * CLOUD_PARALLAX), drawClouds);

    drawScrollingLayer((float)scrollX, [] {
        drawBrickLayer();
        drawPlatforms();
        drawFloatingBlocks();
        drawSmallBlocks();
        drawBushes();
    });

    currentScrollX = 0.0f;
    glFlush();
    if (dumpPath) { writePPM(dumpPath); exit(0); }
    glutSwapBuffers();
}
```

**Drawing order** (back to front): sky → clouds (3 scrolling tile copies) →
main-ground brick face → stepped platforms (brick + grass caps) → floating
crate → ground/tower crates → bushes, each also drawn 3 times as scrolling
tile copies. Bushes are drawn last within their group because in the
reference art they overlap the base of the platforms and the ground rim.

#### Animation loop

```cpp
void update(int) {
    int now = glutGet(GLUT_ELAPSED_TIME);
    double dt = (now - lastTimeMs) / 1000.0;
    lastTimeMs = now;

    if (!paused) {
        speed = speed + ACCEL * dt;
        if (speed > MAX_SPEED) speed = MAX_SPEED;
        scrollX += speed * dt;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);   // reschedule itself, ~60 fps
}
```

`update()` is registered once with `glutTimerFunc` in `main()` and keeps
rescheduling itself. Using `glutGet(GLUT_ELAPSED_TIME)` for `dt` (rather than
assuming a fixed frame time) keeps the scroll speed correct in real seconds
regardless of how fast the timer actually fires. `speed` ramps from
`BASE_SPEED` (130 art units/s) up to `MAX_SPEED` (320) at `ACCEL` (2/s²) -
the "the game gets faster" feel from the Chrome dinosaur game, capped so it
never becomes unreadable.

#### Headless frame dump (`--dump`, `--scroll`)

```bash
./game_macos --dump out.ppm            # render one frame at scrollX=0, exit
./game_macos --dump out.ppm --scroll 367   # render one frame mid-scroll, exit
```

`--dump` renders exactly one frame, writes it to a binary PPM file via
`glReadPixels`, and exits - no window interaction needed. It fires from
inside the very first `display()` call, before `update()` has ever ticked,
so by default it captures the scene at `scrollX = 0` - useful for diffing
against the reference JPEG pixel-by-pixel. `--scroll <units>` seeds the
starting `scrollX`, which was used while developing the scroll to dump
frames at several offsets (including exactly one `TILE_W` apart) and check
the tile seam for gaps or misalignment without needing to interact with a
live window.

#### Input Handling

```cpp
void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0);                              // ESC - quit
    if (key == ' ' || key == 'p' || key == 'P') paused = !paused;
    if (key == 'r' || key == 'R') { scrollX = 0.0; speed = BASE_SPEED; }
}
```

#### Window Management

```cpp
void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
    glutPostRedisplay();
}
```

Because `display()` always projects the fixed 1200x700 virtual canvas onto
whatever the current viewport is, resizing the window rescales the whole
scene uniformly rather than revealing/cropping content.

---

### 2. Makefile - Build Configuration

Unchanged from the original: platform-detected GLUT/OpenGL flags, and the
usual `make` / `make run` / `make debug` / `make clean` / `make rebuild` /
`make help` / `make info` targets. See the Makefile itself for details.

---

## Color Palette Used

| Component        | RGB (float)                  | Hex        |
|-------------------|------------------------------|------------|
| Sky top           | (0.255, 0.643, 0.969)         | #41A4F7 |
| Sky bottom        | (0.447, 0.922, 0.988)         | #72EBFC |
| Haze cloud wash   | (0.560, 1.000, 1.000)         | #8FFFFF |
| Cloud grey        | (0.820, 0.914, 0.929)         | #D1E9ED |
| Cloud cream       | (0.961, 0.996, 0.933)         | #F5FEEE |
| Cloud white       | (0.996, 0.996, 0.996)         | #FEFEFE |
| Sky speck         | (0.596, 0.882, 0.957)         | #98E1F4 |
| Outline (all art) | (0.055, 0.180, 0.208)         | #0E2E35 |
| Grass light       | (0.702, 0.886, 0.478)         | #B3E27A |
| Grass mid         | (0.376, 0.631, 0.506)         | #60A181 |
| Grass dark        | (0.278, 0.553, 0.596)         | #478D98 |
| Brick light       | (0.937, 0.537, 0.439)         | #EF8970 |
| Brick mid         | (0.882, 0.502, 0.451)         | #E18073 |
| Brick mortar      | (0.745, 0.365, 0.329)         | #BE5D54 |
| Brick cap (soil)  | (0.827, 0.459, 0.431)         | #D3756E |
| Bush light/mid/dark | (0.698,0.863,0.471) / (0.443,0.702,0.435) / (0.278,0.553,0.596) | #B2DC78 / #71B36F / #478D98 |
| Crate outline     | (0.176, 0.137, 0.310)         | #2D234F |
| Crate fill        | (1.000, 0.675, 0.451)         | #FFAC73 |

---

## Technical Details

### Graphics API
- **OpenGL 2.x** (Immediate Mode)
- **GLUT** for window management and input
- **Platform**: macOS (Darwin) with Linux support

### Coordinate Systems
- **Art space**: 735 x 412, origin top-left - this is what every drawing
  function in the file actually takes as input, matching the reference
  image's own pixel grid.
- **Canvas space**: 1200 x 700, origin bottom-left - the GL ortho
  projection. `IX`/`IY`/`IW`/`IH` convert art space → canvas space.

### Rendering Technique
- Immediate mode (`glBegin`/`glEnd`), vertex-by-vertex
- Deterministic hashing instead of `rand()` so jittered details (brick
  joints, grass blade heights, bush placement) never change between frames
  or between an interactive run and a `--dump` run - and so every tile copy
  of the scrolling scene is bit-for-bit identical to every other copy
- Flat opaque fills for the cloud masses (see note above) rather than alpha
  blending, to avoid overlap seams
- Infinite scroll via tiling, not by generating new content: the scene is
  one 735-unit-wide tile, drawn 3 times side by side each frame at a
  scrolling offset (see "Scrolling" above) rather than procedurally
  extending the world - simple, and exactly matches a classic looping
  2D-platformer background

### Buffer Management
- Double buffering (`GLUT_DOUBLE`) for the interactive window
- `--dump` reads straight from `GL_BACK` after `glFlush()`, before the swap
- `glutTimerFunc` (not `glutIdleFunc`) drives the animation, at a fixed
  16 ms nominal interval, so the app is idle (no busy-loop) between ticks

---

## Usage Instructions

### Basic Setup

```bash
cd /Users/siam/Desktop/Background
make
make run
```

### In the Application

- **View**: The 2D platformer background scrolls left forever in the window
- **Pause / resume**: Press SPACE or P
- **Reset speed**: Press R (scroll position keeps going, only the speed resets)
- **Exit**: Press ESC key
- **Resize**: Drag window edge to resize - the whole scene rescales

### For Development / Regression Checking

```bash
make clean && make
./game_macos --dump /tmp/frame.ppm                    # frame at scrollX=0
./game_macos --dump /tmp/frame.ppm --scroll 367        # frame mid-scroll
./game_macos --dump /tmp/a.ppm --scroll 0
./game_macos --dump /tmp/b.ppm --scroll 735            # exactly one tile later
```

Compare frames against the reference JPEG, or against each other, with e.g.
Pillow, to check colour/geometry and confirm the tile seam has no gaps after
any edit.

---

## Files Summary

| File | Purpose | Status |
|------|---------|--------|
| `game.cpp` | Main C++ source code | Rewritten to match the reference image, then made to scroll infinitely |
| `Makefile` | Build configuration | Unchanged, verified working |
| `README.md` | User documentation | Updated for scrolling + new controls |
| `code.md` | This file - code documentation | Rewritten to match `game.cpp` |
| `game_macos` | Compiled executable | Rebuilt from the current source |
| `WhatsApp Image 2026-08-07 at 21.22.43.jpeg` | Reference art the scene is matched against | — |

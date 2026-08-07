# GLUT 2D Platformer Game Scene

A 2D platformer game scene rendered using OpenGL and GLUT (OpenGL Utility Toolkit). This program recreates the pixel-art style game environment shown in the reference image, and scrolls it infinitely - like the ground/obstacles in the Chrome "dinosaur" game.

## Features

- **Sky and Clouds**: Blue sky background with layered clouds (soft washes, shadow puffs, cream cumulus banks, one crisp cloud, drifting particles)
- **Floating Blocks**: Orange/red rivetted crates, one hanging in the air
- **Multi-level Platforms**: Green grass platforms at different heights with wispy grass edges
- **Brick Layers**: Detailed, hand-jittered brick pattern for the ground and stepped platforms
- **Bushes**: Two-tone scalloped bushes for environmental detail
- **Tower Structures**: A stepped brick platform forming a tower-like structure
- **Infinite Scrolling**: The whole scene slides left forever and loops seamlessly, with the sky drifting slower than the ground (parallax) and the scroll speed slowly ramping up over time, dino-game style
- **Responsive Rendering**: Window can be resized and the scene adapts

## System Requirements

### macOS
- Xcode Command Line Tools (or GCC/G++ compiler)
- GLUT framework (usually included)
- OpenGL (usually included)

### Linux
- GCC or G++ compiler
- libglut development libraries
- OpenGL development libraries
- Build essentials

### Installation

#### macOS
```bash
# Ensure you have Xcode command line tools
xcode-select --install
```

#### Linux (Debian/Ubuntu)
```bash
sudo apt-get install build-essential freeglut3-dev mesa-common-dev
```

#### Linux (Fedora/RHEL)
```bash
sudo dnf install gcc-c++ glut-devel mesa-devel
```

## Building

### Basic Build
```bash
make
```

### Build and Run
```bash
make run
```

### Debug Build
```bash
make debug
```

### Clean Build Artifacts
```bash
make clean
```

### Full Rebuild
```bash
make rebuild
```

### View Help
```bash
make help
```

## Running

Once built, run the executable directly:

```bash
./game_macos      # macOS
./game_linux      # Linux
./game            # Generic
```

Or use:
```bash
make run
```

## Controls

- **ESC Key**: Exit the program
- **SPACE or P**: Pause / resume the scroll
- **R**: Reset scroll speed back to its starting pace (position keeps scrolling)
- **Window Resize**: The scene will adapt to new window size

## Code Structure

### Drawing Functions
- `drawFilledRect()`: Draw filled rectangles
- `drawRectOutline()`: Draw rectangle outlines
- `drawCircle()`: Draw circles (used for clouds)
- `drawTriangle()`: Draw triangles
- `drawWavyGrass()`: Legacy sine-wave grass helper (grass rims now use `drawBladeBand()` instead)

### Scene Components
- `drawSky()`: Renders the sky background (fixed, never scrolls)
- `drawClouds()`: Renders cloud objects
- `drawFloatingBlocks()`: Renders suspended orange crates
- `drawPlatforms()`: Renders the stepped grass/brick platforms
- `drawBrickLayer()`: Renders the main ground's brick pattern
- `drawBushes()`: Renders the scalloped bushes
- `drawSmallBlocks()`: Renders the crates resting on the ground/tower

### Scrolling
- `scrollX` / `speed`: How far the world has slid, and how fast, in
  art-space units - advanced every tick by `update()`
- `drawScrollingLayer()`: Draws three side-by-side copies of a layer's
  content, offset so the seam between copies always falls off-screen -
  this is what makes the ground and sky loop forever
- `CLOUD_PARALLAX`: How much slower the sky scrolls than the ground (0.35x)

### Main Functions
- `display()`: Main render function called by GLUT - draws the sky once,
  then the cloud and ground layers as scrolling tile runs
- `update()`: Timer callback (~60 fps) that advances `scrollX`/`speed` in
  real time and reschedules itself
- `reshape()`: Handles window resizing
- `keyboard()`: Handles keyboard input (quit / pause / reset)

## Customization

### Change Window Size
Edit `main()` function:
```cpp
glutInitWindowSize(1200, 700);  // Change these values
```

### Modify Colors
Colors are defined as RGB values (0.0 to 1.0). Find `setColor()` calls and adjust:
```cpp
setColor(1.0f, 0.0f, 0.0f);  // Red
setColor(0.0f, 1.0f, 0.0f);  // Green
setColor(0.0f, 0.0f, 1.0f);  // Blue
```

### Add New Elements
Create a new function like:
```cpp
void drawNewElement() {
    setColor(R, G, B);  // Set color
    drawFilledRect(x, y, width, height);  // Draw shape
}
```

Then call it in `display()` function.

## Troubleshooting

### Compilation Errors
If you get GLUT-related errors:
- macOS: Usually works out of the box
- Linux: Install GLUT development libraries
  ```bash
  sudo apt-get install freeglut3-dev
  ```

### Runtime Issues
- If window appears black: Check color values (0.0-1.0 range)
- If nothing appears: Ensure ortho projection is set correctly in `display()`
- If program crashes: Press ESC key to exit gracefully

### Build Information
Check your system configuration:
```bash
make info
```

## File Structure

```
.
├── game.cpp          # Main C++ source code
├── Makefile          # Build configuration
├── README.md         # This file
└── code.md           # Code summary
```

## Future Enhancements

Potential additions:
- Player character/sprite (a "dino" to jump over the platforms)
- Basic physics (gravity, jumping) and collision detection
- Score / distance counter tied to `scrollX`
- Randomized obstacle spacing instead of a fixed repeating tile
- Sound effects (with OpenAL)

## License

Free to use and modify for educational and personal projects.

## Notes

- This is a basic 2D rendering example using GLUT and OpenGL
- It's designed to run on modern systems with OpenGL 2.0+ support
- The code uses immediate mode (glBegin/glEnd) for simplicity
- For production use, consider using modern OpenGL with VBOs and shaders

---

**Created**: 2026-08-07
**Purpose**: Educational GLUT/OpenGL 2D graphics demonstration

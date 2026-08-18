# Desert Scene Game

This project is a small OpenGL desert animation made in C++. It shows a sunny desert scene with:

- a blue sky
- a glowing sun
- drifting clouds
- rolling tumbleweeds
- tall and short cacti
- rocks and sand details
- a girl character who can run and jump

It is a fun example of how a game scene is built from small drawing functions and animation loops.

## Project files

- `desert_scene.cpp` - builds the whole desert scene and animation loop
- `player.h` - public player functions that other code can use
- `player.cpp` - draws the player and handles jumping/running animation
- `game` - compiled executable for running the game

## How to run it

Open a terminal in this project folder and run:

```bash
g++ desert_scene.cpp player.cpp -o game -framework OpenGL -framework GLUT
./game
```

If you already have the `game` executable built, you can just run:

```bash
./game
```

## Controls

- `Esc` = quit the game
- `P` = pause or resume
- `R` = reset the animation

## What the code is doing

### `desert_scene.cpp`
This file creates the world. It draws the sky, hills, cactus plants, clouds, rocks, and the player. It also updates the time so everything looks like it is moving.

### `player.h`
This file is like a menu for the player character. It tells other files which player functions are available, such as:

- update the player
- make the player jump
- draw the player
- check the height of the jump
- change the running speed

### `player.cpp`
This file is the player character itself. It draws the girl’s body, head, arms, legs, dress, and hair. It also handles the jump physics so she rises and then falls back to the ground.

## Beginner-friendly explanation

Think of the code like building with Lego blocks:

- one function draws a cloud
- one function draws a cactus
- one function draws a rock
- one function updates the animation time
- one function draws the whole scene

The game is made by calling all these little drawing functions in the right order.

## Notes

This project is a learning project and is great for understanding:

- C++ functions
- OpenGL drawing commands
- animation timing
- object movement
- simple game loop design

## Good next ideas

You could try adding:

- a score counter
- moving enemies
- sound effects
- a start screen
- an end screen
- a second character or pet

## Summary

This is a simple desert game scene built with C++ and OpenGL. It is a great project for learning how game graphics, animation, and scene drawing work together.

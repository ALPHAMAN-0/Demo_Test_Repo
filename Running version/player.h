

#ifndef PLAYER_H          // include guard: stops the file being pasted in twice
#define PLAYER_H

// Advance the run cycle and the jump physics.
// Call once per frame, dt = seconds since the last frame (0.016f at 60 fps).
void playerUpdate(float dt);

// Start a jump. Ignored if she is already in the air.
// Wire this to the space bar in your keyboard() callback.
void playerJump();

// Draw her standing on ground level y, facing right.
// The jump height is added internally, so pass the ground y, not her feet.
void drawPlayer(float x, float y);

// How far off the ground she currently is, in pixels (0 while running).
// Useful for collision checks -- e.g. did she clear that tumbleweed?
float playerHeight();

// Run speed, in radians of cycle per second (9.0 is a steady jog).
void playerSetSpeed(float radiansPerSecond);

#endif

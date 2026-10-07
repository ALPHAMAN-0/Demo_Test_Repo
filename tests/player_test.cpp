// Tests for the jump logic in "Running version/player.cpp"
// (playerJump / playerUpdate / playerHeight).
//
// player.cpp also holds the OpenGL drawing code, so the test has to be linked
// with GLUT although it never opens a window. From the repository root (macOS,
// like the build command in "Running version/README.md"):
//
//   g++ tests/player_test.cpp "Running version/player.cpp" -o tests/player_test.out -framework OpenGL -framework GLUT -Wno-deprecated-declarations && ./tests/player_test.out

#include <cstdio>

#include "../Running version/player.h"

static int checks = 0;
static int failures = 0;

#define CHECK(cond)                                                \
    do {                                                           \
        checks++;                                                  \
        if (!(cond)) {                                             \
            failures++;                                            \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond);    \
        }                                                          \
    } while (0)

int main()
{
    const float dt = 1.0f / 60.0f;   // one frame at 60 fps
    float peak = 0.0f;
    int frames = 0;

    // she starts on the ground, and running alone does not lift her
    CHECK(playerHeight() == 0.0f);
    playerUpdate(dt);
    CHECK(playerHeight() == 0.0f);

    // a jump leaves the ground with the next update
    playerJump();
    playerUpdate(dt);
    CHECK(playerHeight() > 0.0f);

    // a second jump in mid-air is ignored: the arc stays that of one jump
    // (330^2 / (2 * 900) is about 60 pixels; a double jump would go higher)
    playerJump();
    while (playerHeight() > 0.0f && frames < 600) {
        if (playerHeight() > peak) peak = playerHeight();
        playerUpdate(dt);
        frames++;
    }
    CHECK(playerHeight() == 0.0f);          // she lands exactly on the ground
    CHECK(peak > 54.0f && peak < 62.0f);
    CHECK(frames > 36 && frames < 50);      // about 0.73 s in the air

    // changing the run speed does not affect the jump state
    playerSetSpeed(12.0f);
    playerUpdate(dt);
    CHECK(playerHeight() == 0.0f);

    std::printf("%d/%d checks passed\n", checks - failures, checks);
    return failures == 0 ? 0 : 1;
}

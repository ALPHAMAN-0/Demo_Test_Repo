# Demo_Test_Repo
Test for the player jump logic (macOS):

## Architecture / How it Works

```mermaid
flowchart LR
    A[User] --> B[Demo Test Repo]
    B --> C{Logic}
    C --> D[(Data)]
    C --> E[Output]
    D --> C
```


    g++ tests/player_test.cpp "Running version/player.cpp" -o tests/player_test.out -framework OpenGL -framework GLUT -Wno-deprecated-declarations && ./tests/player_test.out

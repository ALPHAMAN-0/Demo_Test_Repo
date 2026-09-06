# CLAUDE.md

## Build / test / lint
- `Background/`: `make` (build), `make run` (build+run), `make debug`, `make clean`, `make rebuild`, `make help`, `make info` — from `Background/Makefile`
- `Running version/`: no build script; build manually with `g++ desert_scene.cpp player.cpp -o game -framework OpenGL -framework GLUT` (per `Running version/README.md`)
- No test or lint commands found in either directory

## Rules
- `Background/Makefile` auto-selects link flags/executable name by OS (`Darwin` → `game_macos`, `Linux` → `game_linux`, `Windows_NT` → `game_windows.exe`) — don't hardcode one platform's flags
- Compiled binaries (`game_macos`, `game.o`, `Running version/game`, `Running version/desert_scene`, `Running version/player`) are committed in-tree — regenerate via build commands above rather than hand-editing

## Read first
1. README.md
2. Background/README.md
3. Running version/README.md

Architecture: see ARCHITECTURE.md — read before structural changes

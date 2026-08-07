# Makefile for GLUT 2D Game Scene
# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
DEBUG_FLAGS = -g -O0
RELEASE_FLAGS = -O2

# Platform detection
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
    # Linux flags
    GLUT_LIBS = -lglut -lGL -lGLU -lm
    GLUT_INCLUDES = 
    EXECUTABLE = game_linux
endif

ifeq ($(UNAME_S),Darwin)
    # macOS flags
    GLUT_LIBS = -framework GLUT -framework OpenGL
    GLUT_INCLUDES = 
    EXECUTABLE = game_macos
endif

# Default to Linux if not recognized
GLUT_LIBS ?= -lglut -lGL -lGLU -lm
EXECUTABLE ?= game

# Source and object files
SOURCES = game.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = $(EXECUTABLE)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) -o $(TARGET) $(OBJECTS) $(GLUT_LIBS)
	@echo "Build complete! Run with: make run"

# Compile source files
%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) $(GLUT_INCLUDES) -c $< -o $@

# Debug build
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)
	@echo "Debug build complete!"

# Run the program
run: $(TARGET)
	@echo "Running $(TARGET)..."
	./$(TARGET)

# Run with debug info
run-debug: debug
	./$(TARGET)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete!"

# Full rebuild
rebuild: clean all

# Help target
help:
	@echo "GLUT 2D Game Scene - Build System"
	@echo "=================================="
	@echo "Available targets:"
	@echo "  make              - Build the project (default)"
	@echo "  make run          - Build and run the program"
	@echo "  make debug        - Build with debug symbols"
	@echo "  make run-debug    - Build and run with debug info"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make rebuild      - Clean and build"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "System detected: $(UNAME_S)"
	@echo "Executable: $(TARGET)"

# Phony targets
.PHONY: all run debug run-debug clean rebuild help

# Print info
info:
	@echo "System: $(UNAME_S)"
	@echo "Compiler: $(CXX)"
	@echo "GLUT Libraries: $(GLUT_LIBS)"
	@echo "Executable: $(TARGET)"

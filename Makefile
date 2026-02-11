# Makefile for Divine Ascent (Episode 6)
# Standardized for macOS with MacPorts SDL2

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Og -I/opt/local/include
LDFLAGS = -L/opt/local/lib -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer

SRC = main.cpp
OBJ = $(SRC:.cpp=.o)
EXE = shadowrecon

all: $(EXE)

$(EXE): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(EXE) $(LDFLAGS)

run: all
	./$(EXE)

clean:
	rm -f $(EXE) $(OBJ)

.PHONY: all run clean

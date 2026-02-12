CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Og -I. -I/opt/local/include
LDFLAGS = -L/opt/local/lib -lSDL2 -lSDL2_ttf

SRC = main.cpp \
      src/engine/Entity.cpp \
      src/engine/AudioManager.cpp \
      src/gameplay/Actor.cpp \
      src/gameplay/Slug.cpp \
      src/ui/HUD.cpp \
      src/Game.cpp

OBJ = $(SRC:.cpp=.o)
TARGET = shadowrecon

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run

CXX = g++

SDL2_CONFIG = $(firstword $(wildcard /mingw64/bin/sdl2-config) sdl2-config)

CXXFLAGS = -Wall -std=c++17 $(shell $(SDL2_CONFIG) --cflags)

LDFLAGS = $(shell $(SDL2_CONFIG) --libs) -lSDL2_mixer

SRC = main.cpp enemy.cpp game.cpp input.cpp player.cpp audioManager.cpp

TARGET = game


all: $(TARGET)

build: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
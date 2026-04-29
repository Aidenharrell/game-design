CXX = g++
CXXFLAGS = -Wall -std=c++17 $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)

SRC = main.cpp game.cpp input.cpp player.cpp
TARGET = game

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
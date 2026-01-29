PROJ := simple
SRC  := $(PROJ).cpp
EXE  := $(PROJ)

CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17

all: $(EXE)

$(EXE): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

run: $(EXE)
	./$(EXE)

clean:
	rm -f $(EXE)

.PHONY: all run clean

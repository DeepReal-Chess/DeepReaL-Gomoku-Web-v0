CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
SRCDIR = src
OBJDIR = obj

# Source files
BOARD_SRC = $(SRCDIR)/board.cpp $(SRCDIR)/precompute.cpp
SEARCH_SRC = $(SRCDIR)/search.cpp
UCI_SRC = $(SRCDIR)/uci.cpp

# Object files
BOARD_OBJ = $(OBJDIR)/board.o $(OBJDIR)/precompute.o
SEARCH_OBJ = $(OBJDIR)/search.o
UCI_OBJ = $(OBJDIR)/uci.o

# Targets
all: gomoku test demo

gomoku: $(OBJDIR)/main.o $(BOARD_OBJ) $(SEARCH_OBJ) $(UCI_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(OBJDIR)/test.o $(BOARD_OBJ) $(SEARCH_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

demo: $(OBJDIR)/demo.o $(BOARD_OBJ) $(SEARCH_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) gomoku test demo

.PHONY: all clean
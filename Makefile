CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

SRCS = main.cpp MemoryManager.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = turing_machine

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

main.o: main.cpp Compiler.h MacrosForTuring.h TuringMachine.h MemoryManager.h
MemoryManager.o: MemoryManager.cpp MemoryManager.h TuringMachine.h

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

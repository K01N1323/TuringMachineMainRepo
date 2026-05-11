CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2

IMGUI_DIR = vendor/imgui
IMGUI_SRCS = $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
IMGUI_SRCS += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

SRCS = main.cpp MemoryManager.cpp $(IMGUI_SRCS)
OBJS = $(SRCS:.cpp=.o)

LDFLAGS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo vendor/glfw/glfw-3.4.bin.MACOS/lib-arm64/libglfw3.a

TARGET = turing_machine

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -Ivendor/glfw/glfw-3.4.bin.MACOS/include -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

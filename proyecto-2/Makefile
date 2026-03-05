# Makefile for proy2

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -I./include -I./include/imgui -I./include/imgui/backends
LDFLAGS = -L/usr/lib/x86_64-linux-gnu
LDLIBS = -lglfw -ldl -lGL -lX11 -lpthread -lXrandr -lXinerama -lXcursor -lXfixes -lXi

# Source files
SRCS = src/Main.cpp src/glad.c src/stb_image.cpp include/imgui/imgui.cpp include/imgui/imgui_demo.cpp include/imgui/imgui_draw.cpp include/imgui/imgui_tables.cpp include/imgui/imgui_widgets.cpp include/imgui/backends/imgui_impl_glfw.cpp include/imgui/backends/imgui_impl_opengl3.cpp

# Object files
MAIN_OBJS = build/main.o build/glad.o build/stb_image.o
IMGUI_OBJS = build/imgui/imgui.o build/imgui/imgui_demo.o build/imgui/imgui_draw.o build/imgui/imgui_tables.o build/imgui/imgui_widgets.o build/imgui/backends/imgui_impl_glfw.o build/imgui/backends/imgui_impl_opengl3.o
OBJS = $(MAIN_OBJS) $(IMGUI_OBJS)

# Directories
DIRS = build build/imgui build/imgui/backends

# Executable name
TARGET = build/proy2

.PHONY: all build clean

all: build

build: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p build
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

build/main.o: src/Main.cpp 
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/glad.o: src/glad.c
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/stb_image.o: src/stb_image.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/imgui/%.o: include/imgui/%.cpp
	@mkdir -p build/imgui
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/imgui/backends/%.o: include/imgui/backends/%.cpp
	@mkdir -p build/imgui/backends
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build

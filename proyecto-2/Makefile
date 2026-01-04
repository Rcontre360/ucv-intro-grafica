# Makefile for proy2

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -I./include -I./include/imgui -I./include/imgui/backends
LDFLAGS = -L/usr/lib/x86_64-linux-gnu
LDLIBS = -lglfw -ldl -lGL -lX11 -lpthread -lXrandr -lXinerama -lXcursor -lXfixes -lXi

# Source files
SRCS = src/main.cpp src/glad.c include/imgui/imgui.cpp include/imgui/imgui_demo.cpp include/imgui/imgui_draw.cpp include/imgui/imgui_tables.cpp include/imgui/imgui_widgets.cpp include/imgui/backends/imgui_impl_glfw.cpp include/imgui/backends/imgui_impl_opengl3.cpp

# Object files
OBJS = $(patsubst src/%.cpp,build/%.o,$(filter src/%.cpp,$(SRCS)))
OBJS += $(patsubst src/%.c,build/%.o,$(filter src/%.c,$(SRCS)))
OBJS += $(patsubst include/imgui/%.cpp,build/imgui/%.o,$(filter include/imgui/%.cpp,$(SRCS)))
OBJS += $(patsubst include/imgui/backends/%.cpp,build/imgui/backends/%.o,$(filter include/imgui/backends/%.cpp,$(SRCS)))

# Directories
DIRS = build build/imgui build/imgui/backends

# Executable name
TARGET = build/proy2

.PHONY: all build clean

all: build

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(sort $(OBJS)) $(LDLIBS)

$(OBJS): | $(DIRS)

$(DIRS):
	mkdir -p $(DIRS)

build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/%.o: src/%.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/imgui/%.o: include/imgui/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/imgui/backends/%.o: include/imgui/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build

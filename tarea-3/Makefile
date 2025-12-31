# Makefile for stb_image project

# Compiler
CC = g++

# Compiler flags
CFLAGS = -std=c++11 -Wall -I. -O3

# Source files
SRCS = src/main.cpp src/stb_libs.cpp

# Object files
OBJS = $(notdir $(SRCS:.cpp=.o))

# Executable name
TARGET = tarea3

# Build directory
BUILD_DIR = build

build: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(addprefix $(BUILD_DIR)/, $(OBJS))
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

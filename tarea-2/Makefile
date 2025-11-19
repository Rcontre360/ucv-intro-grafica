CXX = g++
CXXFLAGS = -std=c++17 -pedantic -O3 -x c++ -w
INCLUDES = -I./include \
           -I./include/stb \
           -I./include/glm \
           -I./include/GLFW \
           -I./include/glad \
           -I./include/assimp \
           -I./include/imgui
LIBS = -lglfw -lGL -lGLU

TARGET = tarea
OBJ_DIR = obj

SRCS = src/glad.c \
       src/main.cpp \
       src/PixelRender.cpp \
       include/imgui/backends/imgui_impl_glfw.cpp \
       include/imgui/backends/imgui_impl_opengl3.cpp \
       include/imgui/imgui.cpp \
       include/imgui/imgui_demo.cpp \
       include/imgui/imgui_draw.cpp \
       include/imgui/imgui_tables.cpp \
       include/imgui/imgui_widgets.cpp

OBJS = $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
OBJS := $(OBJS:%.c=$(OBJ_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJS) $(LDIR) $(LIBS) -o $@

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/$(dir $<)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@ -MMD

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/$(dir $<)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@ -MMD

$(OBJ_DIR):
	@echo "Creating object directory: $(OBJ_DIR)"
	@mkdir -p $(OBJ_DIR)

.PHONY: clean
clean:
	@echo "Cleaning up build artifacts..."
	@rm -rf $(OBJ_DIR) $(TARGET)

-include $(OBJS:.o=.d)

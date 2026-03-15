#pragma once

#include "imgui/imgui.h"
#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include "State.h"
#include "../utils/FileLoader.h"
#include "portable-file-dialogs.h"

enum class UIMovementMode { FPS = 0, GOD = 1 };

struct UISceneContext {
    int width;
    int height;
    UIMovementMode& movementMode;
    bool& isFalling;
    float& verticalVelocity;
    float targetY;
    glm::vec3 initialCameraPos;
    GLFWwindow* window;
    bool& firstMouse;
};

namespace UI {

    // Helper for Object-level texture loading (affects all submeshes)
    inline void textureMapButton(const char* label, Object* obj, bool isNormalMap = false) {
        if (ImGui::Button(label, ImVec2(-1, 0))) {
            auto res = pfd::open_file(label, ".", { "Image Files", "*.png *.jpg *.jpeg *.bmp *.tga" }).result();
            if (!res.empty()) {
                GLuint tex = FileLoader::loadTexture(res[0]);
                if (tex) {
                    for (auto sm : obj->submeshes) {
                        if (isNormalMap) {
                            if (sm->normalMap) {
                                glDeleteTextures(1, &sm->normalMap);
                                sm->normalMap = 0;
                            }
                            sm->normalMap = tex;
                        } else {
                            if (sm->diffuseMap) {
                                glDeleteTextures(1, &sm->diffuseMap);
                                sm->diffuseMap = 0;
                            }
                            if (sm->ambientMap) {
                                glDeleteTextures(1, &sm->ambientMap);
                                sm->ambientMap = 0;
                            }
                            if (sm->specularMap) {
                                glDeleteTextures(1, &sm->specularMap);
                                sm->specularMap = 0;
                            }
                            sm->diffuseMap = tex;
                        }
                    }
                }
            }
        }
    }

    inline void renderLoadingScreen(int width, int height) {
        ImGui::SetNextWindowPos(ImVec2(width / 2.0f - 120.0f, height / 2.0f - 50.0f));
        ImGui::SetNextWindowSize(ImVec2(240, 100));
        ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
        ImGui::SetCursorPos(ImVec2(80, 40));
        ImGui::Text("...Loading...");
        ImGui::End();
    }

    inline void renderSettings(State* appState, UISceneContext& ctx) {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(300, (float)ctx.height));
        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

        if (appState) {
            if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Global Attenuation", &appState->useAttenuation);
                ImGui::Separator();
                for (int i = 0; i < (int)appState->lights.size(); i++) {
                    std::string label = "Light " + std::to_string(i + 1);
                    if (ImGui::TreeNode(label.c_str())) {
                        Light* l = appState->lights[i];
                        ImGui::Checkbox("Enabled", &l->enabled);
                        ImGui::SliderFloat("Intensity", &l->intensity, 0.0f, 5.0f);
                        ImGui::ColorEdit3("Diffuse",  &l->diffuse.x);
                        ImGui::ColorEdit3("Ambient",  &l->ambient.x);
                        ImGui::ColorEdit3("Specular", &l->specular.x);
                        ImGui::SliderFloat("Anim Speed", &l->animationSpeed, 0.0f, 5.0f);
                        const char* modes[] = { "Phong", "Blinn-Phong", "Flat" };
                        int currentMode = (int)l->shadingMode;
                        if (ImGui::Combo("Shading", &currentMode, modes, IM_ARRAYSIZE(modes)))
                            l->shadingMode = (ShadingMode)currentMode;
                        ImGui::TreePop();
                    }
                }
            }

            if (ImGui::CollapsingHeader("Movement")) {
                const char* moveModes[] = { "FPS", "GOD" };
                int currentMoveMode = (int)ctx.movementMode;
                if (ImGui::Combo("Mode", &currentMoveMode, moveModes, IM_ARRAYSIZE(moveModes))) {
                    UIMovementMode newMode = (UIMovementMode)currentMoveMode;
                    if (ctx.movementMode == UIMovementMode::GOD && newMode == UIMovementMode::FPS) {
                        float curY = Camera::getInstance().position.y;
                        if (curY > ctx.targetY) {
                            ctx.isFalling = true;
                            ctx.verticalVelocity = 0.0f;
                        } else if (curY < ctx.targetY - 0.01f) {
                            Camera::getInstance().position = ctx.initialCameraPos;
                        }
                    }
                    ctx.movementMode = newMode;
                }
                if (ImGui::Button("Mouse Camera")) {
                    glfwSetInputMode(ctx.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    ctx.firstMouse = true;
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Press ESC to release");
            }

            if (ImGui::CollapsingHeader("Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::BeginChild("obj_list", ImVec2(-1, 200), false);
                for (auto obj : appState->objects) {
                    bool sel = obj->isSelected;
                    if (ImGui::Selectable(obj->name.c_str(), sel)) {
                        for (auto o : appState->objects) o->isSelected = false;
                        if (!sel) obj->isSelected = true;
                    }
                }
                ImGui::EndChild();
            }

            Object* selected = nullptr;
            for (auto obj : appState->objects)
                if (obj->isSelected) { selected = obj; break; }

            if (selected) {
                std::string editLabel = "Edit - " + selected->name;
                if (ImGui::CollapsingHeader(editLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    textureMapButton("Set Diffuse Map", selected, false);
                    textureMapButton("Set Bump Map", selected, true);

                    ImGui::Separator();
                    const char* sMappings[] = { "Standard", "Spherical", "Squared" };
                    const char* oMappings[] = { "Position", "Normal" };
                    int currentS = selected->submeshes.empty() ? 0 : selected->submeshes[0]->sMappingMode;
                    if (ImGui::Combo("s-mapping", &currentS, sMappings, IM_ARRAYSIZE(sMappings)))
                        for (auto sm : selected->submeshes) sm->sMappingMode = currentS;
                    int currentO = selected->submeshes.empty() ? 0 : selected->submeshes[0]->oMappingMode;
                    if (ImGui::Combo("o-mapping", &currentO, oMappings, IM_ARRAYSIZE(oMappings)))
                        for (auto sm : selected->submeshes) sm->oMappingMode = currentO;
                }
            }
        }

        ImGui::End();
    }
}

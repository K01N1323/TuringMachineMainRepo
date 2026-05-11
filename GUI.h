#ifndef GUI_H
#define GUI_H

#include "TuringMachine.h"
#include "MemoryManager.h"
#include "Compiler.h"

#include "vendor/imgui/imgui.h"
#include "vendor/imgui/backends/imgui_impl_glfw.h"
#include "vendor/imgui/backends/imgui_impl_opengl3.h"

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

class TuringMachineGUI {
public:
    TuringMachineGUI(TuringMachine& tm, MemoryManager& mem)
        : tm_(tm), mem_(mem), window_(nullptr), running_(false), autoPlay_(false), speedMs_(100) {}

    bool Init() {
        if (!glfwInit()) return false;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        window_ = glfwCreateWindow(1280, 720, "3-Tape Turing Machine ALU", nullptr, nullptr);
        if (!window_) return false;

        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        running_ = true;
        return true;
    }

    void Run() {
        while (!glfwWindowShouldClose(window_) && running_) {
            glfwPollEvents();

            if (autoPlay_) {
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastStepTime_).count() > speedMs_) {
                    if (!tm_.Step()) {
                        autoPlay_ = false;
                    }
                    lastStepTime_ = now;
                }
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            RenderUI();

            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window_, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window_);
        }
    }

    ~TuringMachineGUI() {
        if (window_) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            glfwDestroyWindow(window_);
            glfwTerminate();
        }
    }

private:
    TuringMachine& tm_;
    MemoryManager& mem_;
    GLFWwindow* window_;
    bool running_;
    bool autoPlay_;
    int speedMs_;
    std::chrono::steady_clock::time_point lastStepTime_;

    void RenderUI() {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Turing Machine", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Text("Current State: %s", tm_.GetCurrentState().c_str());
        ImGui::Separator();

        if (ImGui::Button(autoPlay_ ? "Pause" : "Play")) autoPlay_ = !autoPlay_;
        ImGui::SameLine();
        if (ImGui::Button("Step")) tm_.Step();
        ImGui::SameLine();
        ImGui::SliderInt("Speed (ms)", &speedMs_, 1, 1000);
        
        ImGui::Separator();

        // Рисуем ленты
        for (int i = 0; i < 3; ++i) {
            ImGui::Text("Tape %d:", i);
            ImGui::BeginChild((std::string("Tape") + std::to_string(i)).c_str(), ImVec2(0, 80), true, ImGuiWindowFlags_HorizontalScrollbar);
            
            const auto& tape = tm_.GetTape(i);
            int head = tm_.GetHead(i);
            
            // Определяем окно просмотра
            int minPos = head - 20;
            int maxPos = head + 20;
            
            for (const auto& [pos, sym] : tape) {
                if (pos < minPos) minPos = pos;
                if (pos > maxPos) maxPos = pos;
            }

            for (int p = minPos; p <= maxPos; ++p) {
                char sym = '_';
                if (tape.find(p) != tape.end()) sym = tape.at(p);
                
                if (p == head) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                }
                
                std::string label = std::string(1, sym) + "##t" + std::to_string(i) + "p" + std::to_string(p);
                ImGui::Button(label.c_str(), ImVec2(30, 30));
                
                if (p == head) {
                    ImGui::PopStyleColor();
                }
                
                if (p < maxPos) ImGui::SameLine();
            }
            ImGui::EndChild();
        }

        ImGui::Separator();
        ImGui::Text("Memory Preview (Decimal):");
        
        // Выводим значения всех пользовательских переменных
        const auto& varNames = mem_.GetVariableNames();
        bool hasVars = false;
        for (const auto& name : varNames) {
            // Игнорируем временные переменные, созданные компилятором
            if (name.find("temp_") == 0 || name.find("const_") == 0) continue;
            
            hasVars = true;
            try {
                int val = mem_.GetDecimalValue(tm_, name);
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s = %d", name.c_str(), val);
            } catch(...) {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s not available", name.c_str());
            }
        }
        
        if (!hasVars) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No variables requested");
        }

        ImGui::End();
    }
};

#endif

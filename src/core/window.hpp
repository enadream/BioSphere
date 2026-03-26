#pragma once

// User-defined headers first
#include "events/event.hpp"

// External libraries
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp> // Added for glm::ivec2

#include <string>
#include <functional>
#include <cstdint>

// The Window class manages only the OS-level window and event forwarding.
// Rendering, input, and logic layers are handled elsewhere.

class Window {
public: // functions
    using EventCallbackFn = std::function<void(Event&)>;

    Window(int32_t width, int32_t height, const std::string title);
    ~Window();
    
    // --- Core loop ---
    void OnUpdate();
    void PollEvents(){ glfwPollEvents(); }
    void SwapBuffers(){ glfwSwapBuffers(m_Window); }
    bool ShouldClose() const { return glfwWindowShouldClose(m_Window); }

    // --- Getters ---
    int32_t GetWidth() const { return m_Data.Width; }
    int32_t GetHeight() const { return m_Data.Height; }
    GLFWwindow* GetNativeWindow() const { return m_Window; }

    bool IsVSync() const { return m_Data.VSync; }
    bool IsFullScreen() const { return m_Data.IsFullScreen; }
    bool IsCursorLocked() const { return m_Data.IsCursorLocked; }

    // --- Setters ---
    void SetEventCallback(const EventCallbackFn& callback);
    void SetTitle(const std::string& title);
    void SetVSync(bool enabled);
    void SetFullScreen(bool fullScreen);
    // Resize the OpenGL Viewport (useful when resizing FrameBuffers)
    void SetViewport(int32_t width, int32_t height);
    // Critical for FPS Games: Locks cursor to center and hides it
    void SetCursorMode(bool locked);
    
private: // functions
    // --- GLFW callbacks ---
    static void FramebufferSizeCallback(GLFWwindow* window, int32_t width, int32_t height);
    static void KeyCallback(GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t mods);
    static void CharCallback(GLFWwindow* window, uint32_t keycode);
    static void MouseButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t mods);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void WindowCloseCallback(GLFWwindow* window);

    // Global error callback for GLFW
    static void GLFWErrorCallback(int32_t error, const char* description);

private: // variables
    struct WindowData {
        std::string Title; 
        int32_t Width, Height;
        bool IsFullScreen = false;
        bool VSync = false;
        bool IsCursorLocked = false;

        EventCallbackFn EventCallback;

        WindowData(int32_t width, int32_t height, const std::string title) 
            : Title(title), Width(width), Height(height) {}
    };

    WindowData m_Data;
    GLFWwindow* m_Window = nullptr;

    // For fullscreen toggling restoration
    GLFWmonitor* m_Monitor = nullptr;
    const GLFWvidmode* m_VideoMode = nullptr;

    // Compact storage using GLM
    glm::ivec2 m_WindowedPos = {0, 0};
    glm::ivec2 m_WindowedSize = {0, 0};
};

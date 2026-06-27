#include "core/window.hpp"
#include "core/log.hpp"
#include "events/application_event.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"

#include <stdexcept>

void Window::GLFWErrorCallback(int32_t error, const char* description){
    LOG_ERROR("GLFW Error (%d): %s", error, description);
}

Window::Window(int32_t width, int32_t height, const std::string title) 
    : m_Data(width, height, title){
    LOG_INFO("Creating window '%s' (%d, %d)", title.c_str(), width, height);

    // --- Init GLFW ---
    glfwSetErrorCallback(GLFWErrorCallback);

    if (!glfwInit()){
        LOG_FATAL("Failed to initialize GLFW!");
        throw std::runtime_error("Failed to initialize GLFW!");
    }
    // Request OpenGL 4.6 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 1); // MSAA
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // --- Create Window ---
    m_Window = glfwCreateWindow(m_Data.Width, m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
    if (!m_Window){
        glfwTerminate();
        LOG_FATAL("Failed to create GLFW window!");
        throw std::runtime_error("Failed to create GLFW window!");
    }

    glfwMakeContextCurrent(m_Window);
    glfwSetWindowUserPointer(m_Window, &m_Data);

    // Load OpenGL pointers via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(m_Window);
        glfwTerminate();
        LOG_FATAL("Failed to initialize GLAD!");
        throw std::runtime_error("Failed to initialize GLAD!");
    }
    
    // Verify OpenGL version
    LOG_INFO("OpenGL Info:");
    LOG_INFO("  Vendor:   %s", (const char*)glGetString(GL_VENDOR));
    LOG_INFO("  Renderer: %s", (const char*)glGetString(GL_RENDERER));
    LOG_INFO("  Version:  %s", (const char*)glGetString(GL_VERSION));

    // --- Set up event callbacks ---
    glfwSetFramebufferSizeCallback(m_Window, FramebufferSizeCallback);
    glfwSetKeyCallback(m_Window, KeyCallback);
    glfwSetCharCallback(m_Window, CharCallback);
    glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
    glfwSetScrollCallback(m_Window, ScrollCallback);
    glfwSetCursorPosCallback(m_Window, CursorPosCallback);
    glfwSetWindowCloseCallback(m_Window, WindowCloseCallback);

    // store monitor + mode (for fullscreen toggle)
    m_Monitor = glfwGetPrimaryMonitor();
    m_VideoMode = glfwGetVideoMode(m_Monitor);

    // Initial viewport
    SetVSync(false);

    LOG_INFO("Window created successfully.");
}

Window::~Window() {
    glfwDestroyWindow(m_Window);
    glfwTerminate();
    LOG_INFO("Window destroyed and GLFW terminated.");
}

void Window::OnUpdate() {
    glfwPollEvents();
    glfwSwapBuffers(m_Window);
}

// --- Setters ---
void Window::SetEventCallback(const EventCallbackFn& callback){
    m_Data.EventCallback = callback;
}
void Window::SetTitle(const std::string& title){
    m_Data.Title = title;
    glfwSetWindowTitle(m_Window, m_Data.Title.c_str());
}
void Window::SetVSync(bool enabled){
    glfwSwapInterval(enabled ? 1 : 0);
    m_Data.VSync = enabled;
}
void Window::SetFullScreen(bool full_screen){
    if (full_screen == m_Data.IsFullScreen) return;

    if (full_screen) {
        glfwGetWindowPos(m_Window, &m_WindowedPos.x, &m_WindowedPos.y);
        glfwGetWindowSize(m_Window, &m_WindowedSize.x, &m_WindowedSize.y);

        // Switch to fullscreen
        glfwSetWindowMonitor(m_Window, m_Monitor, 0, 0,
                             m_VideoMode->width, m_VideoMode->height,
                             m_VideoMode->refreshRate);
    } else {
        // Switch to windowed mode, center the window
        // int xpos = (m_VideoMode->width - m_Data.Width) / 2;
        // int ypos = (m_VideoMode->height - m_Data.Height) / 2;
        // glfwSetWindowMonitor(m_Window, nullptr, xpos, ypos, m_Data.Width, m_Data.Height, 0);
        glfwSetWindowMonitor(m_Window, nullptr,
                        m_WindowedPos.x, m_WindowedPos.y, 
                        m_WindowedSize.x, m_WindowedSize.y, 0);
    }

    m_Data.IsFullScreen = full_screen;
}

void Window::SetViewport(int32_t width, int32_t height){
    glViewport(0, 0, width, height);
}

void Window::SetCursorMode(bool locked){
    if (locked) {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        m_Data.IsCursorLocked = true;
    } else {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_Data.IsCursorLocked = false;
    }
}

// --- GLFW Callbacks ---
void Window::FramebufferSizeCallback(GLFWwindow* window, int32_t width, int32_t height){
    auto& data = *(WindowData*)(glfwGetWindowUserPointer(window));
    data.Width = width;
    data.Height = height;

    WindowResizeEvent event(width, height);
    data.EventCallback(event);
}

void Window::KeyCallback(GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t mods){ 
    auto& data = *(WindowData*)glfwGetWindowUserPointer(window);
    switch (action){
        case GLFW_PRESS: {
            KeyPressedEvent event(key, false);
            data.EventCallback(event);
            break;
        }
        case GLFW_RELEASE: {
            KeyReleasedEvent event(key);
            data.EventCallback(event);
            break;
        }
        case GLFW_REPEAT: {
            KeyPressedEvent event(key, true);
            data.EventCallback(event);
            break;
        }
    }
}

void Window::CharCallback(GLFWwindow* window, uint32_t keycode){
    auto& data = *(WindowData*)glfwGetWindowUserPointer(window);
    KeyTypedEvent event(keycode);
    data.EventCallback(event);
}

void Window::MouseButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t mods){
    auto& data = *(WindowData*)glfwGetWindowUserPointer(window);
    switch (action){
        case GLFW_PRESS: {
            MouseButtonPressedEvent event(button);
            data.EventCallback(event);
            break;
        }
        case GLFW_RELEASE: {
            MouseButtonReleasedEvent event(button);
            data.EventCallback(event);
            break;
        }
    }
}

void Window::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset){
    auto& data = *(WindowData*)glfwGetWindowUserPointer(window);
    MouseScrolledEvent event((float)xoffset, (float)yoffset);
    data.EventCallback(event);
}

void Window::CursorPosCallback(GLFWwindow* window, double xpos, double ypos){
    auto& data = *(WindowData*)glfwGetWindowUserPointer(window);
    MouseMovedEvent event((float)xpos, (float)ypos);
    data.EventCallback(event);
}

void Window::WindowCloseCallback(GLFWwindow* window){
    auto& data = *(WindowData*)glfwGetWindowUserPointer(window);
    WindowCloseEvent event;
    data.EventCallback(event);
}
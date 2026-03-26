#pragma once

#include "core/layer_stack.hpp"
#include "events/event.hpp"
#include "events/application_event.hpp"

#include <cstdint>
#include <string>
#include <memory>

// Forward Declarations (Optimizes compile time)
class Window;
class Timer;


class Application {
public: // functions
    Application(int32_t width, int32_t height, const std::string& name);
    virtual ~Application();

    // Main loop
    void Run();
    void SetRunning(bool isRunning);

    // Global event entry point
    void OnEvent(Event& event);

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);

    static Application& Get() { return *s_Instance; }
    Window& GetWindow() { return *m_Window; }

    // Delete copy/move to enforce Singleton pattern
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

private: // functions
    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);


private: // variables
    static Application* s_Instance;

    // core systems
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<Timer> m_Timer;
    LayerStack m_LayerStack;
 
    bool m_Running = true;
    bool m_Minimized = false;

    std::string m_Name;
};

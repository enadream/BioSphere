#include "core/application.hpp"
#include "core/layer.hpp"
#include "core/timer.hpp"
#include "core/window.hpp"
#include "core/log.hpp"

Application* Application::s_Instance = nullptr;

Application::Application(int32_t width, int32_t height, const std::string& name) : m_Name(name){
    // Initialize Logger first!
    Log::Init();

    if (s_Instance){
        LOG_FATAL("Application already exists!");
        throw std::runtime_error("Application already exists!");
    }

    s_Instance = this;

    // Create core systems
    m_Window = std::make_unique<Window>(width, height, m_Name);
    m_Window->SetEventCallback([this](Event& e){this->OnEvent(e); });
    
    m_Timer = std::make_unique<Timer>();
    LOG_INFO("Application created: %s", m_Name.c_str());
}

Application::~Application(){
    LOG_INFO("Shutting down application: %s", m_Name.c_str());
}

void Application::Run(){
    while (m_Running){
        m_Timer->Update();
        float deltaTime = m_Timer->GetDeltaTime();

        if (!m_Minimized){
            for (auto& layer : m_LayerStack)
                layer->OnUpdate(deltaTime);
            for (auto& layer : m_LayerStack)
                layer->OnRender();
        }

        m_Window->OnUpdate(); // Swap buffers, poll events
    }
}

void Application::SetRunning(bool isRunning){
    m_Running = isRunning;
}

void Application::OnEvent(Event& event){
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });

    // Propagate through layers (from top to bottom)
    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it){
        if (event.Handled)
            break;
        (*it)->OnEvent(event);
    }
}

bool Application::OnWindowClose(WindowCloseEvent& e){
    m_Running = false;
    return true;
}

bool Application::OnWindowResize(WindowResizeEvent& e){
    if (e.GetWidth() == 0 || e.GetHeight() == 0) {
        m_Minimized = true;
        return false;
    }

    m_Minimized = false;
    return false;
}

void Application::PushLayer(Layer* layer){
    m_LayerStack.PushLayer(layer);
}

void Application::PushOverlay(Layer* overlay){
    m_LayerStack.PushOverlay(overlay);
}
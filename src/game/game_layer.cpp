#include "game/game_layer.hpp"
#include "core/application.hpp"
#include "core/window.hpp"
#include "core/input.hpp"
#include "core/log.hpp"
#include "io/save_manager.hpp"
#include "world/terrain_generator.hpp"

#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace {
    // Shared between the world streamer and the spawn-height query so terrain matches.
    const Seed256 kWorldSeed{941456789ULL, 423654321ULL, 111222333ULL, 444555666ULL};
    constexpr const char* kSaveRoot       = "data/save";
    constexpr const char* kPlayerSaveName = "player";
}

// --- Construction ---

GameLayer::GameLayer()
    : Layer("GameLayer"),
      m_Camera(glm::vec3(0.0f, 0.0f, 0.0f), 1280, 720),
      m_World(ChunkStreamer::Config{
          .renderDistance = DEFAULT_RENDER_DISTANCE,
          .workerCount    = [] {
              // Reserve one core for the main thread; use the rest for chunk work.
              const uint32_t hw = std::max(1u, std::thread::hardware_concurrency());
              return hw > 1u ? hw - 1u : 1u;
          }(),
          .seed           = kWorldSeed,
          .worldDir       = "data/world"
      })
{}

// --- OnAttach ---

void GameLayer::OnAttach() {
    // Lock cursor for FPS-style mouse look
    Application::Get().GetWindow().SetCursorMode(true);
    m_CursorLocked = true;
    m_FirstMouse   = true;

    // Restore previous camera state, or drop onto the terrain if no save exists.
    SaveManager::Get().Init(kSaveRoot);
    const bool restored = SaveManager::Get().Load(SaveType::Player, kPlayerSaveName);
    if (restored) {
        m_Camera.SetPosition(m_SavedPosition.Value());
        const glm::vec2 r = m_SavedRotation.Value();
        m_Camera.SetOrientation(r.x, r.y);
        LOG_INFO("[GameLayer] Restored camera at (%.1f, %.1f, %.1f)",
                 m_SavedPosition.Value().x, m_SavedPosition.Value().y, m_SavedPosition.Value().z);
    } else {
        // No save - sample terrain at spawn coords and place camera a short distance above.
        TerrainGenerator terrain(kWorldSeed);
        const float spawnX = 0.0f;
        const float spawnZ = 0.0f;
        const float groundY = terrain.GetHeight(spawnX, spawnZ);
        m_Camera.SetPosition(glm::vec3(spawnX, groundY + 4.0f, spawnZ));
        m_Camera.SetOrientation(-90.0f, -10.0f);
        LOG_INFO("[GameLayer] Fresh spawn at terrain height %.1f", groundY);
    }

    // Initialize the world (loads world-header.bin, starts worker threads)
    m_World.Init();

    // Initialize GPU resources (VBO, VAO, SSBOs, shaders)
    m_Renderer.Init(DEFAULT_RENDER_DISTANCE);

    // Trigger the first streaming tick so workers begin generating chunks
    m_Camera.Update();
    m_World.Update(m_Camera.GetPosition());
}

// --- OnDetach ---

void GameLayer::OnDetach() {
    // Snapshot camera state into the save wrappers before shutdown.
    m_SavedPosition = m_Camera.GetPosition();
    m_SavedRotation = glm::vec2(m_Camera.GetYaw(), m_Camera.GetPitch());
    SaveManager::Get().SaveAsync(SaveType::Player, kPlayerSaveName);
    SaveManager::Get().FlushPendingSaves();

    m_Renderer.Shutdown();
    m_World.Shutdown();
    Application::Get().GetWindow().SetCursorMode(false);
}

// --- OnUpdate ---

void GameLayer::OnUpdate(float deltaTime) {
    // Keyboard movement
    if (Input::IsKeyPressed(GLFW_KEY_W))
        m_Camera.ProcessMovement(CameraMovement::Forward, deltaTime);
    if (Input::IsKeyPressed(GLFW_KEY_S))
        m_Camera.ProcessMovement(CameraMovement::Backward, deltaTime);
    if (Input::IsKeyPressed(GLFW_KEY_A))
        m_Camera.ProcessMovement(CameraMovement::Left, deltaTime);
    if (Input::IsKeyPressed(GLFW_KEY_D))
        m_Camera.ProcessMovement(CameraMovement::Right, deltaTime);
    if (Input::IsKeyPressed(GLFW_KEY_E))
        m_Camera.ProcessMovement(CameraMovement::Up, deltaTime);
    if (Input::IsKeyPressed(GLFW_KEY_Q))
        m_Camera.ProcessMovement(CameraMovement::Down, deltaTime);

    // Boost speed with Shift
    if (Input::IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
        m_Camera.Settings.MovementSpeed = 200.0f;
    else
        m_Camera.Settings.MovementSpeed = 25.0f;

    m_Camera.Update();
    m_Camera.CalculateFrustum();

    if (m_World.Update(m_Camera.GetPosition()))
        m_Renderer.SyncChunks(m_World);
    m_Renderer.UpdateUploads(m_World);

    // Update window title once per second with FPS, sphere count, and VSync state.
    m_FpsAccum += deltaTime;
    ++m_FpsFrames;
    if (m_FpsAccum >= 1.0f) {
        const int      fps     = static_cast<int>(m_FpsFrames / m_FpsAccum);
        const uint32_t spheres = m_Renderer.GetSphereCount();
        std::string title = "BioSphere  [" + std::to_string(fps) + " FPS | "
                          + std::to_string(spheres) + " spheres"
                          + (m_VSync ? " | VSync ON" : "") + "]";
        Application::Get().GetWindow().SetTitle(title);
        m_FpsAccum  = 0.0f;
        m_FpsFrames = 0;
    }
}

// --- OnRender ---

void GameLayer::OnRender() {
    glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_Renderer.Render(m_Camera);
}

// --- OnEvent ---

void GameLayer::OnEvent(Event& event) {
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<WindowResizeEvent>(
        [this](WindowResizeEvent& e) { return OnWindowResized(e); });
    dispatcher.Dispatch<KeyPressedEvent>(
        [this](KeyPressedEvent& e) { return OnKeyPressed(e); });
    dispatcher.Dispatch<MouseMovedEvent>(
        [this](MouseMovedEvent& e) { return OnMouseMoved(e); });
}

bool GameLayer::OnWindowResized(WindowResizeEvent& e) {
    const int32_t w = e.GetWidth();
    const int32_t h = e.GetHeight();
    if (w == 0 || h == 0) return false; // minimized
    glViewport(0, 0, w, h);
    m_Camera.SetViewportSize(w, h);
    m_Camera.Update();
    return false;
}

bool GameLayer::OnKeyPressed(KeyPressedEvent& e) {
    if (e.GetKeyCode() == GLFW_KEY_ESCAPE && !e.IsRepeat()) {
        m_CursorLocked = !m_CursorLocked;
        Application::Get().GetWindow().SetCursorMode(m_CursorLocked);
        m_FirstMouse = true;  // reset delta on re-lock to avoid jump
    }
    if (e.GetKeyCode() == GLFW_KEY_V && !e.IsRepeat()) {
        m_VSync = !m_VSync;
        Application::Get().GetWindow().SetVSync(m_VSync);
    }
    return false;
}

bool GameLayer::OnMouseMoved(MouseMovedEvent& e) {
    if (!m_CursorLocked) return false;

    const float x = e.GetX();
    const float y = e.GetY();

    if (m_FirstMouse) {
        m_LastMousePos = {x, y};
        m_FirstMouse   = false;
        return false;
    }

    const float dx =  (x - m_LastMousePos.x) * m_Camera.Settings.MouseSensitivity;
    const float dy = -(y - m_LastMousePos.y) * m_Camera.Settings.MouseSensitivity;
    m_LastMousePos = {x, y};

    m_Camera.Rotate(dx, dy);
    return false;
}


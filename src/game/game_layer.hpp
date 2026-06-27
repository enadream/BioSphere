#pragma once

#include "core/layer.hpp"
#include "events/application_event.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"
#include "renderer/camera.hpp"
#include "game/world_renderer.hpp"
#include "world/world_handler.hpp"
#include "io/save_wrapper.hpp"
#include "io/field_id.hpp"

#include <glm/glm.hpp>

class GameLayer : public Layer {
public:
    GameLayer();
    ~GameLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnEvent(Event& event) override;

private:
    bool OnKeyPressed(KeyPressedEvent& e);
    bool OnMouseMoved(MouseMovedEvent& e);
    bool OnWindowResized(WindowResizeEvent& e);

    Camera        m_Camera;
    WorldHandler  m_World;
    WorldRenderer m_Renderer;

    // Persisted via SaveManager - registered on construction, written on OnDetach.
    // Rotation packs yaw (x) and pitch (y) in degrees.
    SaveWrapper<glm::vec3, FieldID::Player::Position> m_SavedPosition;
    SaveWrapper<glm::vec2, FieldID::Player::Rotation> m_SavedRotation;

    glm::vec2 m_LastMousePos{0.0f, 0.0f};
    bool      m_CursorLocked = true;
    bool      m_FirstMouse   = true;

    float    m_FpsAccum  = 0.0f;
    int      m_FpsFrames = 0;
    bool     m_VSync     = false;
};

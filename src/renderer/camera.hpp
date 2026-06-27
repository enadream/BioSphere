#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "io/save_wrapper.hpp"
#include "physics/frustum.hpp"

enum class CameraMovement : uint8_t {
    Forward, Backward, Left, Right, Up, Down
};

// Tunable settings for the camera
struct CameraSettings {
    // Physics / Controls
    //SaveWrapper<float, ID> Speed{25.0f};
    float MovementSpeed = 25.0f;
    float MouseSensitivity = 0.1f;
    float ZoomSensitivity = 2.0f;

    // Lens / Optics
    float FovY = 45.0f;        // Field of View in Degrees
    float NearClip = 0.1f;
    float FarClip = 10000.0f; // Large draw distance for Voxels

    // Constraints
    float PitchLimit = 89.0f;
    bool ConstrainPitch = true;
};

class Camera {
public:
    Camera( glm::vec3 position,
            int32_t width, int32_t height,
            float yaw = -90.0f, float pitch = -30.0f);

    // Recalculate camera vectors and matrices
    void Update();

    // --- Input Processing ---
    // Uses internal Settings.MovementSpeed
    void ProcessMovement(CameraMovement direction, float deltaTime);
    // Uses internal Settings.MouseSensitivity
    void Rotate(float yawOffset, float pitchOffset);
     // Uses internal Settings.ZoomSensitivity
    void ProcessMouseScroll(float yOffset);

    // Updates frustum planes based on current PV matrix
    void CalculateFrustum();
    
    // --- Getters ---

    // View/Projection matrices
    glm::mat4 GetViewMatrix() const noexcept;
    glm::mat4 GetProjMatrix() const noexcept;
    glm::mat4 GetProjViewMatrix() const noexcept;

    // Camera attributes
    glm::vec3 GetPosition() const noexcept { return m_Position; }
    void SetPosition(const glm::vec3& pos) noexcept { m_Position = pos; }
    void SetPosition(float x, float y, float z) noexcept { m_Position = {x, y, z}; }

    float GetYaw()   const noexcept { return m_Yaw; }
    float GetPitch() const noexcept { return m_Pitch; }
    // Set yaw and pitch; caller must Update() to refresh vectors and matrices.
    void  SetOrientation(float yaw, float pitch) noexcept { m_Yaw = yaw; m_Pitch = pitch; }

    glm::vec3 GetFront() const noexcept { return m_Front; }
    glm::vec3 GetUp() const noexcept { return m_Up; }
    glm::vec3 GetRight() const noexcept { return m_Right; }

    // Accessors for Settings
    float GetFovYDeg() const noexcept { return Settings.FovY; }
    float GetFovYRad() const noexcept { return glm::radians(Settings.FovY); }
    void SetFovY(float fov) { Settings.FovY = fov; Update(); }
    float GetNear() const noexcept { return Settings.NearClip; }
    float GetFar() const noexcept { return Settings.FarClip; }

    int32_t GetWidth() const noexcept { return m_Width; }
    int32_t GetHeight() const noexcept { return m_Height; }
    float GetAspectRatio() const noexcept { return static_cast<float>(m_Width) / m_Height; }
    void SetViewportSize(int32_t width, int32_t height) noexcept;
    
    // Frustum
    Frustum& GetFrustum() noexcept { return m_Frustum; }
    const Frustum& GetFrustum() const noexcept { return m_Frustum; }

public: // variables
    // Public settings - easy to change from GameLayer
    CameraSettings Settings;

private: // variables
    // Position & orientation
    glm::vec3 m_Position;   // Camera position in world space
    glm::vec3 m_Front;      // Forward vector of the camera
    glm::vec3 m_Up;         // Up vector of the camera
    glm::vec3 m_Right;      // Right vector of the camera
    glm::vec3 m_WorldUp;    // World up direction

    // Projection
    glm::mat4 m_ViewMatrix;
    glm::mat4 m_ProjMatrix;
    glm::mat4 m_ProjViewMatrix;

    int32_t m_Width;
    int32_t m_Height;

    // Orientation angles
    float m_Yaw;   // horizontal axis in degrees
    float m_Pitch; // vertical axis in degrees

    // Frustum
    Frustum m_Frustum;    
};
#pragma once

#include <glm/glm.hpp>

// Static input class - No init required
class Input {
public:
    // Check if a specific key is currently held down
    static bool IsKeyPressed(int keycode);

    // Check if a mouse button is currently held down
    static bool IsMouseButtonPressed(int button);

    // Get the current mouse position in window coordinates
    static glm::vec2 GetMousePosition();
};
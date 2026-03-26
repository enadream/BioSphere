#include "core/input.hpp"
#include "core/application.hpp"
#include "core/window.hpp"

#include <GLFW/glfw3.h>

bool Input::IsKeyPressed(int keycode){
    // Get the native GLWF window from the Application Singleton
    int state = glfwGetKey(Application::Get().GetWindow().GetNativeWindow(), keycode);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsMouseButtonPressed(int button){
    int state = glfwGetMouseButton(Application::Get().GetWindow().GetNativeWindow(), button);
    return state == GLFW_PRESS;
}

glm::vec2 Input::GetMousePosition(){
    double xpos, ypos;
    glfwGetCursorPos(Application::Get().GetWindow().GetNativeWindow(), &xpos, &ypos);
    return {(float)xpos, (float)ypos};
}

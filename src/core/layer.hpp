#pragma once

#include "events/event.hpp"
#include "core/log.hpp"

#include <string>


// Base class for all layers in the application.
// Layers are self-contained modules that can be pushed onto the application's layer stack.
class Layer {
public:
    Layer(const std::string& debugName = "Layer") : m_DebugName(debugName) {
        LOG_INFO("Layer created: %s", m_DebugName.c_str());
    }

    virtual ~Layer() = default;

    // Called when the layer is attached to the application.
    virtual void OnAttach() {}
    // Called when the layer is detached from the application.
    virtual void OnDetach() {}
    // Called every frame to update the layer's state.
    virtual void OnUpdate(float deltaTime) {}
    // Called every frame to render the layer's content.
    virtual void OnRender() {}
    // Called when an event is dispatched to this layer.
    virtual void OnEvent(Event& event) {}

    // Getter for the layer's name, useful for debugging.
    inline const std::string& GetName() const { return m_DebugName; }
    
protected:
    std::string m_DebugName;
};

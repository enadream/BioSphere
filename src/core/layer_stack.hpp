#pragma once

#include "core/layer.hpp"
#include <cstdint>
#include <vector>


// The LayerStack is a container for layers.
// It manages the order and ownership of the layers.
// It allows the application to iterate over layers for updates, rendering, and event handling.
class LayerStack {
public:
    LayerStack() = default;
    ~LayerStack();

    // Pushes a layer onto the back of the stack.
    void PushLayer(Layer* layer);
    // Pushes an overlay to the front of the stack.
    // Overlays are rendered on top of regular layers and receive events first.
    void PushOverlay(Layer* overlay);
    // Pops a layer from the stack.
    void PopLayer(Layer* layer);
    // Pops an overlay from the stack.
    void PopOverlay(Layer* overlay);

    // Iterators to allow for range-based for loops.
    std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
    std::vector<Layer*>::iterator end() { return m_Layers.end(); }
    std::vector<Layer*>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
    std::vector<Layer*>::reverse_iterator rend() { return m_Layers.rend(); }

    // Constant iterators
    std::vector<Layer*>::const_iterator begin() const { return m_Layers.begin(); }
    std::vector<Layer*>::const_iterator end() const { return m_Layers.end(); }
    std::vector<Layer*>::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
    std::vector<Layer*>::const_reverse_iterator rend() const { return m_Layers.rend(); }

private:
    std::vector<Layer*> m_Layers;
    uint32_t m_LayerInsertIndex = 0;
};
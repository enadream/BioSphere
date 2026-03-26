#include "layer_stack.hpp" // Related header first
#include "core/log.hpp" // User-defined headers

// Standard library headers last
#include <algorithm>
#include <vector>

LayerStack::~LayerStack() {

    for (Layer* layer : m_Layers)
    {
        layer->OnDetach();
        delete layer;
    }

    LOG_INFO("LayerStack destroyed.");
}

void LayerStack::PushLayer(Layer* layer) {
    layer->OnAttach();
    m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
    m_LayerInsertIndex++;
    LOG_TRACE("Pushed layer to stack: %s", m_Layers[m_LayerInsertIndex - 1]->GetName().c_str());
}

void LayerStack::PushOverlay(Layer* overlay) {
    overlay->OnAttach();
    m_Layers.emplace_back(overlay);
    LOG_TRACE("Pushed overlay to stack: %s", m_Layers.back()->GetName().c_str());
}

void LayerStack::PopLayer(Layer* layer) {
    auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);

    if (it != m_Layers.begin() + m_LayerInsertIndex) {
        layer->OnDetach();
        delete layer; 
        m_Layers.erase(it);
        m_LayerInsertIndex--;
        LOG_TRACE("Popped layer from stack.");
    }
}

void LayerStack::PopOverlay(Layer* overlay) {
    
    auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);

    if (it != m_Layers.end()) {
        overlay->OnDetach();
        delete overlay;
        m_Layers.erase(it);
        LOG_TRACE("Popped overlay from stack.");
    }
}

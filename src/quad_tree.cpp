#include "quad_tree.hpp"


QuadTree::QuadTree() {
    for (uint32_t i = 0; i < childeren.size(); i++){
        childeren[i] = nullptr; 
    }
    p_data = nullptr;
}
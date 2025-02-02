#ifndef QUAD_TREE_HPP
#define QUAD_TREE_HPP

#include <vector>
#include <array>
#include <stdint.h>
#include <glm/glm.hpp>

#include "bound_box.hpp"

// grid size has to match the deepness
struct LeafGrid {
    std::vector<std::vector<glm::vec3>> positions; // 24 bytes
};

class QuadTree {
public: // functions
    QuadTree();
    ~QuadTree();

public: // variables
    // 00, 01, 10, 11
    std::array<QuadTree*, 4> childeren; // 32 bytes
    LeafGrid *p_data; // 8 bytes
    BoundBox boundBox; // 24 bytes

private: // variables

private: // functions
   
};

#endif
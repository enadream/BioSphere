#include "world/sphere.hpp"

#include <format>

void Sphere::ToString(std::string& outStr) const {    
    // print position
    uint8_t x, z;
    int16_t y; 
    Position.GetCoordinates(x, y, z);
    std::format_to(std::back_inserter(outStr), "    Position: X:{}, Y:{}, Z:{}\n", x, y, z);
    // print chunk type and flags as binary
    std::format_to(std::back_inserter(outStr), "    Flags: {:b}\n", ChunkTypeAndFlags);
    // ambient as binary
    std::format_to(std::back_inserter(outStr), "    AmbientOcclusion: {:b}\n", AmbientOcclusion);
    // print light vectors
    for (uint32_t i = 0; i < Lights.size(); i++){
        std::format_to(std::back_inserter(outStr), "    LightVec[{}]: R:{}, G{}, B{}\n", i, Lights[i].R, Lights[i].G, Lights[i].B);
    }
}
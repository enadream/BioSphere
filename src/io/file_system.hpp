#pragma once

#include "core/log.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>


namespace fs = std::filesystem;

class FileSystem {
public:
    static bool WriteBinary(const std::string& path, const uint8_t* data, size_t size) {
        // Ensure directory exist
        fs::path p(path);
        if (p.has_parent_path()) {
            std::error_code ec;
            fs::create_directories(p.parent_path(), ec);
            if (ec) {
                LOG_ERROR("[FileSystem] Failed to create directories for: %s (%s)",
                          path.c_str(), ec.message().c_str());
                return false;
            }
        }

        std::ofstream out(path, std::ios::binary);
        if (!out.is_open()) {
            LOG_ERROR("[FileSystem] Failed to open file for writing: %s", path.c_str());
            return false;
        }

        out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));

        if (!out.good()) {
            LOG_ERROR("[FileSystem] Failed to write %zu bytes to: %s", size, path.c_str());
            return false;
        }

        return true;
    }

    static bool ReadBinary(const std::string& path, std::vector<uint8_t>& buffer){
        // Open directly -- skip exists() check to avoid TOCTOU race
        std::ifstream in(path, std::ios::binary | std::ios::ate);
        if (!in.is_open()) {
            LOG_WARN("[FileSystem] Failed to open file for reading: %s", path.c_str());
            return false;
        }

        const std::streamsize size = in.tellg();
        if (size < 0) {
            LOG_ERROR("[FileSystem] Failed to determine file size: %s", path.c_str());
            return false;
        }

        in.seekg(0, std::ios::beg);
        buffer.resize(static_cast<size_t>(size));

        if (!in.read(reinterpret_cast<char*>(buffer.data()), size)) {
            LOG_ERROR("[FileSystem] Failed to read %zu bytes from: %s", static_cast<size_t>(size), path.c_str());
            buffer.clear(); // don't leave a half-filled buffer
            return false;
        }

        return true;
    }

    static bool Exists(const std::string& path) {
        return fs::exists(path);
    }
};
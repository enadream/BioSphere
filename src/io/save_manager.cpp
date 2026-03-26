#include "io/save_manager.hpp"
#include "io/field_archive.hpp"
#include "io/file_system.hpp"

#include "core/log.hpp"
#include <cassert>

SaveManager& SaveManager::Get() {
    static SaveManager instance; // Meyer's singleton - thread-safe, initialized on first call
    return instance;
}

void SaveManager::Init(const std::string& rootDirectory) {
    m_RootDir = rootDirectory;
}

// -- Registry -----------------------------------------------------------------
void SaveManager::Register(IWrapperHandle* handle) {
    const uint32_t id = handle->GetFieldID();
    auto [it, inserted] = m_Registry.emplace(id, handle);
    // Duplicate registration = two SaveWrapper objects share the same FieldID.
    if (!inserted) {
        LOG_ERROR("[SaveManager] Duplicate FieldID registration! ID: 0x%X", id);
        assert(false);
    }
    (void)it;
}

void SaveManager::Unregister(uint32_t fieldId) {
    m_Registry.erase(fieldId);
}

// -- FlushPendingSaves ---------------------------------------------------------
void SaveManager::FlushPendingSaves() {
    for (auto& f : m_PendingSaves)
        if (f.valid()) f.wait();
    m_PendingSaves.clear();
}

// -- SaveAsync -----------------------------------------------------------------
void SaveManager::SaveAsync(SaveType type, const std::string& filename) {
    // 1. Snapshot all matching wrappers on the main thread
    //    (safe - we own the values, no race with game logic after this point)
    FieldArchive archive;
    for (const auto& [id, handle] : m_Registry) {
        if (GetSaveType(id) == type)
            archive.Write(*handle);
    }

    // 2. Encrypt / compress + write on background thread
    m_PendingSaves.push_back(
        std::async(std::launch::async,
            [this, type, filename, data = archive.TakeBuffer()]() mutable
        {
            switch (type) {
                case SaveType::Player:
                    Encrypt(data);
                    break;
                case SaveType::Map:
                    Compress(data);
                    break;
                case SaveType::World:
                    break; // plain binary, no processing
                default:
                    LOG_WARN("[SaveManager] SaveAsync: unhandled SaveType %u -- aborting write.", 
                        static_cast<uint8_t>(type));
                    return; // do NOT write garbage to disk
            }

            const std::string path = GetPath(type, filename);
            if (FileSystem::WriteBinary(path, data.data(), data.size()))
                LOG_INFO("[SaveManager] Saved: %s", path.c_str());
            else
                LOG_ERROR("[SaveManager] Failed to save: %s", path.c_str());
        })
    );
}

// -- Load ---------------------------------------------------------------------
bool SaveManager::Load(SaveType type, const std::string& filename) {
    const std::string path = GetPath(type, filename);
    std::vector<uint8_t> data;

    if (!FileSystem::ReadBinary(path, data)) return false;

    switch (type) {
        case SaveType::Player:
            Decrypt(data);
            break;
        case SaveType::Map:
            Decompress(data);
            break;
        case SaveType::World:
            break; // plain binary, no processing
        default:
            LOG_WARN("[SaveManager] Load: unhandled SaveType %u -- aborting load.",
                static_cast<uint8_t>(type));
            return false; // do NOT feed unknown data into wrappers
    }

    FieldArchive archive;
    if (!archive.LoadBuffer(data))  {
        LOG_ERROR("[SaveManager] Corrupt archive: %s", path.c_str());
        return false;
    }

    // Push loaded values into all matching registered wrappers.
    // Wrappers not present in the file keep their default values - safe upgrades.
    for (auto& [id, handle] : m_Registry) {
        if (GetSaveType(id) == type)
            archive.Read(*handle);
    }
    return true;
}

// -- LoadAsync -----------------------------------------------------------------
std::future<bool> SaveManager::LoadAsync(SaveType type, const std::string& filename) {
    // All registered wrappers MUST outlive this future.
    return std::async(std::launch::async, [this, type, filename]() -> bool {
        return Load(type, filename);
    });
}

// -- Path helper ---------------------------------------------------------------
std::string SaveManager::GetPath(SaveType type, const std::string& filename) {
    std::string folder = m_RootDir + "/";
    switch (type) {
        case SaveType::Player: folder += "players/"; break;
        case SaveType::World:  folder += "worlds/";  break;
        case SaveType::Map:    folder += "maps/";    break;
        default:               folder += "misc/";    break;
    }
    return folder + filename;
}

// -- Encryption placeholder (swap for AES-256-GCM) ----------------------------
void SaveManager::Encrypt(std::vector<uint8_t>& data) {
constexpr uint8_t KEY = 0x5A;
    for (auto& b : data) b ^= KEY;

}
void SaveManager::Decrypt(std::vector<uint8_t>& data) {
    Encrypt(data); // XOR is symmetric
}

// -- Compression placeholder (swap for LZ4) ------------------------------------
void SaveManager::Compress(std::vector<uint8_t>& data) { 
    // TODO LZ4
}

void SaveManager::Decompress(std::vector<uint8_t>& data) { 
    // TODO LZ4
}
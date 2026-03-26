#pragma once

#include "io/wrapper_handle.hpp"
#include "io/field_id.hpp"
#include <string>
#include <future>
#include <vector>
#include <unordered_map>
#include <cassert>

class SaveManager {
public:
    static SaveManager& Get();
    void Init(const std::string& rootDirectory);

    // -- Registry -------------------------------------------------------------
    // Called automatically by SaveWrapper constructor/destructor.
    // One slot per FieldID - double-registration fires an assert (it's a bug).
    void Register  (IWrapperHandle* handle);
    void Unregister(uint32_t fieldId);

    // -- Save / Load -----------------------------------------------------------
    // Operates on ALL registered wrappers matching 'type'.
    // No object parameter needed - wrappers registered themselves.

    // Snapshot + encrypt/compress on main thread, write on background thread.
    void SaveAsync(SaveType type, const std::string& filename);

    // Blocking load - decrypt/decompress + push values into registered wrappers.
    bool Load(SaveType type, const std::string& filename);

    // Non-blocking load - caller must ensure registered wrappers outlive the future.
    std::future<bool> LoadAsync(SaveType type, const std::string& filename);

    // Block until all background saves complete. Called automatically on destruction.
    void FlushPendingSaves();

private:
    SaveManager()  = default;
    ~SaveManager() { FlushPendingSaves(); }

    SaveManager(const SaveManager&)            = delete;
    SaveManager& operator=(const SaveManager&) = delete;

    std::string          GetPath(SaveType type, const std::string& filename);
    void Encrypt(std::vector<uint8_t>& data);
    void Decrypt(std::vector<uint8_t>& data);
    void Compress(std::vector<uint8_t>& data);
    void Decompress(std::vector<uint8_t>& data);

    // FieldID -> handle pointer (one slot per ID)
    std::unordered_map<uint32_t, IWrapperHandle*> m_Registry;

    std::string                    m_RootDir;
    std::vector<std::future<void>> m_PendingSaves;
};
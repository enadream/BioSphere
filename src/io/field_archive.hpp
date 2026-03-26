#pragma once

#include "io/wrapper_handle.hpp"
#include "core/log.hpp"
#include <vector>
#include <unordered_map>
#include <cstring>
#include <cstdint>

// Binary format per field: [ FieldID : uint32 | Length : uint32 | Bytes : Length ]
class FieldArchive {
public:
    // -- Write -----------------------------------------------------------------
    void Write(const IWrapperHandle& handle) {
        WriteRaw(handle.GetFieldID(), handle.RawPtr(), handle.RawSize());
    }

    const std::vector<uint8_t>& GetBuffer() const { return m_WriteBuffer; }
    // move out, leaves archive empty
    std::vector<uint8_t> TakeBuffer() { return std::move(m_WriteBuffer); }

    void Clear() {
        m_WriteBuffer.clear();
        m_ReadIndex.clear();
        m_ReadData.clear();
    }

    // -- Read ------------------------------------------------------------------
    // Indexes the flat buffer by FieldID. Must be called once before Read().
    // This function steals the data
    bool LoadBuffer(std::vector<uint8_t>& data) {
        m_ReadData  = std::move(data);
        m_ReadIndex.clear();
        size_t offset = 0;

        while (offset + 8 <= m_ReadData.size()) {
            uint32_t id = 0, len = 0;
            std::memcpy(&id,  m_ReadData.data() + offset,     4);
            std::memcpy(&len, m_ReadData.data() + offset + 4, 4);
            offset += 8;

            if (offset + len > m_ReadData.size()) {
                LOG_ERROR("[FieldArchive] Corrupt data: field 0x%X claims %u bytes, "
                          "only %zu remain", id, len, m_ReadData.size() - offset);
                return false;
            }
            m_ReadIndex[id] = { m_ReadData.data() + offset, len };
            offset += len;
        }
        return true;
    }

    // Returns false if field not present -> wrapper keeps its default value.
    bool Read(IWrapperHandle& handle) const {
        auto it = m_ReadIndex.find(handle.GetFieldID());
        if (it == m_ReadIndex.end()) return false;
        handle.LoadRaw(it->second.Ptr, it->second.Size);
        return true;
    }

    bool HasField(uint32_t id) const { return m_ReadIndex.count(id) > 0; }

private:
    struct FieldEntry { const uint8_t* Ptr; uint32_t Size; };

    void WriteRaw(uint32_t id, const void* data, size_t size) {
        const uint32_t len     = static_cast<uint32_t>(size);
        const size_t   oldSize = m_WriteBuffer.size();
        m_WriteBuffer.resize(oldSize + 8 + len);
        std::memcpy(m_WriteBuffer.data() + oldSize,     &id,  4);
        std::memcpy(m_WriteBuffer.data() + oldSize + 4, &len, 4);
        std::memcpy(m_WriteBuffer.data() + oldSize + 8, data, len);
    }

    std::vector<uint8_t>                     m_WriteBuffer;
    std::vector<uint8_t>                     m_ReadData;
    std::unordered_map<uint32_t, FieldEntry> m_ReadIndex;
};
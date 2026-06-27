#pragma once

#include "world/chunk.hpp"

#include <cstddef>
#include <list>
#include <memory>
#include <unordered_map>

// LRU cache of Chunk objects.
// When capacity is reached, Insert() evicts the least-recently-used entry and
// returns its unique_ptr so the caller can save it to disk if IsDirty.
// Capacity 0 means unbounded (no eviction).
// Not thread-safe - must be accessed from a single thread.
class ChunkCache {
public:
    explicit ChunkCache(size_t capacity = 0);

    // Look up a resident chunk. Returns nullptr on miss. Touch makes it MRU.
    Chunk*       Find(ChunkCoordinates coords);
    const Chunk* Find(ChunkCoordinates coords) const;

    bool   Contains(ChunkCoordinates coords) const;
    size_t Size()     const noexcept;
    size_t Capacity() const noexcept;

    // Insert chunk; take ownership. If at capacity, evicts LRU and returns it.
    // Returns nullptr when no eviction occurred.
    std::unique_ptr<Chunk> Insert(std::unique_ptr<Chunk> chunk);

    // Remove a specific entry and return ownership. Returns nullptr on miss.
    std::unique_ptr<Chunk> Remove(ChunkCoordinates coords);

    void Clear();

    // Iterate all resident chunks. fn signature: void(Chunk&)
    template<typename Fn>
    void ForEach(Fn&& fn) {
        for (auto& [key, entry] : m_Map)
            fn(*entry.chunk);
    }

    static uint64_t MakeKey(ChunkCoordinates coords) noexcept;

private:
    void Touch(uint64_t key);

    size_t m_Capacity;

    struct Entry {
        std::unique_ptr<Chunk> chunk;
        std::list<uint64_t>::iterator lruIt;
    };

    std::unordered_map<uint64_t, Entry> m_Map;
    std::list<uint64_t> m_LRU; // front = MRU, back = LRU
};

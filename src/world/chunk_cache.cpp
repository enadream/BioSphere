#include "world/chunk_cache.hpp"

ChunkCache::ChunkCache(size_t capacity)
    : m_Capacity(capacity)
{}

uint64_t ChunkCache::MakeKey(ChunkCoordinates coords) noexcept {
    return coords.GetKey();
}

Chunk* ChunkCache::Find(ChunkCoordinates coords) {
    const uint64_t key = MakeKey(coords);
    auto it = m_Map.find(key);
    if (it == m_Map.end()) return nullptr;
    Touch(key);
    return it->second.chunk.get();
}

const Chunk* ChunkCache::Find(ChunkCoordinates coords) const {
    const uint64_t key = MakeKey(coords);
    auto it = m_Map.find(key);
    if (it == m_Map.end()) return nullptr;
    return it->second.chunk.get();
}

bool ChunkCache::Contains(ChunkCoordinates coords) const {
    return m_Map.count(MakeKey(coords)) > 0;
}

size_t ChunkCache::Size() const noexcept     { return m_Map.size(); }
size_t ChunkCache::Capacity() const noexcept { return m_Capacity; }

std::unique_ptr<Chunk> ChunkCache::Insert(std::unique_ptr<Chunk> chunk) {
    const uint64_t key = MakeKey(chunk->GetCoordinates());

    // If already cached, just update and touch.
    auto existing = m_Map.find(key);
    if (existing != m_Map.end()) {
        existing->second.chunk = std::move(chunk);
        Touch(key);
        return nullptr;
    }

    std::unique_ptr<Chunk> evicted;

    // Evict LRU entry when at capacity.
    if (m_Capacity > 0 && m_Map.size() >= m_Capacity) {
        const uint64_t lruKey = m_LRU.back();
        m_LRU.pop_back();
        auto lruIt = m_Map.find(lruKey);
        evicted = std::move(lruIt->second.chunk);
        m_Map.erase(lruIt);
    }

    // Insert at front of LRU list.
    m_LRU.push_front(key);
    m_Map.emplace(key, Entry{std::move(chunk), m_LRU.begin()});

    return evicted;
}

std::unique_ptr<Chunk> ChunkCache::Remove(ChunkCoordinates coords) {
    const uint64_t key = MakeKey(coords);
    auto it = m_Map.find(key);
    if (it == m_Map.end()) return nullptr;

    m_LRU.erase(it->second.lruIt);
    auto ptr = std::move(it->second.chunk);
    m_Map.erase(it);
    return ptr;
}

void ChunkCache::Clear() {
    m_Map.clear();
    m_LRU.clear();
}

void ChunkCache::Touch(uint64_t key) {
    auto it = m_Map.find(key);
    if (it == m_Map.end()) return;
    // Move to front of LRU list.
    m_LRU.erase(it->second.lruIt);
    m_LRU.push_front(key);
    it->second.lruIt = m_LRU.begin();
}

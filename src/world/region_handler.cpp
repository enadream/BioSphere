#include "world/region_handler.hpp"
#include "io/file_system.hpp"
#include "core/log.hpp"

#include <cinttypes>
#include <cstring>
#include <format>
#include <fstream>

// --- RegionContent ---

RegionContent::RegionContent(std::string headPath, std::string dataPath)
    : m_HeadPath(std::move(headPath)), m_DataPath(std::move(dataPath))
{}

void RegionContent::ReadHeader() {
    std::vector<uint8_t> buffer;
    if (!FileSystem::ReadBinary(m_HeadPath, buffer))
        return; // absent -> new region, m_Metas stays empty

    if (buffer.size() % sizeof(RegionChunkMeta) != 0) {
        LOG_ERROR("[RegionContent] Corrupt header (size %zu not a multiple of 16): %s",
                  buffer.size(), m_HeadPath.c_str());
        return;
    }

    const size_t count = buffer.size() / sizeof(RegionChunkMeta);
    m_Metas.resize(count);
    std::memcpy(m_Metas.data(), buffer.data(), buffer.size());
}

void RegionContent::SaveHeader() {
    const size_t byteSize = m_Metas.size() * sizeof(RegionChunkMeta);
    if (byteSize == 0) {
        // Nothing to save yet; ensure the directory exists for future writes.
        FileSystem::EnsureParentDirectory(m_HeadPath);
        m_Modified = false;
        return;
    }
    if (!FileSystem::WriteBinary(m_HeadPath,
            reinterpret_cast<const uint8_t*>(m_Metas.data()), byteSize)) {
        LOG_ERROR("[RegionContent] Failed to save header: %s", m_HeadPath.c_str());
        return;
    }
    m_Modified = false;
}

bool RegionContent::FindChunk(uint64_t key, uint64_t& outOffset) const {
    uint64_t idx;
    if (!BinarySearch(key, idx))
        return false;
    outOffset = m_Metas[idx].Offset;
    return true;
}

bool RegionContent::WriteChunk(Chunk& chunk) {
    std::vector<uint8_t> blob;
    chunk.Serialize(blob);

    FileSystem::EnsureParentDirectory(m_DataPath);
    std::ofstream out(m_DataPath, std::ios::binary | std::ios::app);
    if (!out.is_open()) {
        LOG_ERROR("[RegionContent] Cannot open reg file for append: %s", m_DataPath.c_str());
        return false;
    }

    // tellp on an app-mode stream returns the end of the file, which is where the
    // new record will land. Lets us skip the separate ifstream-for-size open.
    const uint64_t offset = static_cast<uint64_t>(out.tellp());

    out.write(reinterpret_cast<const char*>(blob.data()),
              static_cast<std::streamsize>(blob.size()));
    if (!out.good()) {
        LOG_ERROR("[RegionContent] Write failed: %s", m_DataPath.c_str());
        return false;
    }
    out.close();

    // Upsert meta entry; maintain sorted order. The header is NOT flushed here -
    // the in-memory index is the source of truth until an explicit SaveHeader call
    // (shutdown / periodic flush). That cuts ~7000 header rewrites per fast move.
    const uint64_t key = chunk.GetCoordinates().GetKey();
    uint64_t idx;
    if (BinarySearch(key, idx)) {
        m_Metas[idx].Offset = offset;
    } else {
        m_Metas.insert(m_Metas.begin() + static_cast<ptrdiff_t>(idx),
                       RegionChunkMeta{key, offset});
    }

    m_Modified = true;
    return true;
}

std::unique_ptr<Chunk> RegionContent::ReadChunk(uint64_t key) const {
    uint64_t offset;
    if (!FindChunk(key, offset)) {
        LOG_WARN("[RegionContent] Key %" PRIu64 " not in header.", key);
        return nullptr;
    }

    std::ifstream file(m_DataPath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("[RegionContent] Cannot open reg file: %s", m_DataPath.c_str());
        return nullptr;
    }

    file.seekg(static_cast<std::streamoff>(offset));
    if (!file.good()) {
        LOG_ERROR("[RegionContent] Seek to %" PRIu64 " failed in: %s", offset, m_DataPath.c_str());
        return nullptr;
    }

    // Read the 12-byte record header (key + chunkSize) to learn total payload size.
    uint8_t hdr[12];
    if (!file.read(reinterpret_cast<char*>(hdr), 12)) {
        LOG_ERROR("[RegionContent] Failed to read record header from: %s", m_DataPath.c_str());
        return nullptr;
    }

    uint32_t chunkSize = 0;
    std::memcpy(&chunkSize, hdr + 8, 4);

    // Sanity-check before allocating: a corrupted offset from an old race condition can
    // produce a garbage chunkSize that would trigger std::bad_alloc and kill the worker thread.
    constexpr uint32_t kMaxChunkBytes =
        static_cast<uint32_t>(CHUNK_SIZE) * static_cast<uint32_t>(CHUNK_SIZE) * 512u * 32u + 64u;
    if (chunkSize == 0 || chunkSize > kMaxChunkBytes) {
        LOG_ERROR("[RegionContent] Implausible chunkSize %" PRIu32 " at offset %" PRIu64 " in: %s"
                  " - skipping (stale/corrupt record)", chunkSize, offset, m_DataPath.c_str());
        return nullptr;
    }

    // Full record = 12-byte header + chunkSize-byte payload.
    std::vector<uint8_t> buffer(12 + chunkSize);
    std::memcpy(buffer.data(), hdr, 12);
    if (!file.read(reinterpret_cast<char*>(buffer.data() + 12), chunkSize)) {
        LOG_ERROR("[RegionContent] Failed to read chunk payload from: %s", m_DataPath.c_str());
        return nullptr;
    }

    // Deserialize overwrites coordinates from the key embedded in the buffer.
    auto chunk = std::make_unique<Chunk>(0, 0);
    chunk->Deserialize(buffer, 0);
    return chunk;
}

bool RegionContent::BinarySearch(uint64_t key, uint64_t& outIndex) const {
    if (m_Metas.empty()) {
        outIndex = 0;
        return false;
    }

    int64_t lo = 0;
    int64_t hi = static_cast<int64_t>(m_Metas.size()) - 1;

    while (lo <= hi) {
        const int64_t mid = lo + (hi - lo) / 2;
        if (m_Metas[mid].Key == key) {
            outIndex = static_cast<uint64_t>(mid);
            return true;
        }
        if (m_Metas[mid].Key < key)
            lo = mid + 1;
        else
            hi = mid - 1;
    }

    outIndex = static_cast<uint64_t>(lo);
    return false;
}

// --- RegionHandler ---

RegionHandler::RegionHandler(uint64_t id, ChunkCoordinates regionPos, const std::string& regDir)
    : m_ID(id), m_RegionPos(regionPos), m_RegDir(regDir)
{}

RegionHandler::~RegionHandler() {
    delete m_Context;
}

RegionHandler::RegionHandler(RegionHandler&& other) noexcept
    : m_ID(other.m_ID),
      m_RegionPos(other.m_RegionPos),
      m_RegDir(std::move(other.m_RegDir)),
      m_Context(other.m_Context)
{
    other.m_Context = nullptr;
}

RegionHandler& RegionHandler::operator=(RegionHandler&& other) noexcept {
    if (this != &other) {
        delete m_Context;
        m_ID       = other.m_ID;
        m_RegionPos = other.m_RegionPos;
        m_RegDir   = std::move(other.m_RegDir);
        m_Context  = other.m_Context;
        other.m_Context = nullptr;
    }
    return *this;
}

void RegionHandler::Load() {
    if (m_Context) return;
    m_Context = new RegionContent(HeadPath(m_RegDir, m_ID), DataPath(m_RegDir, m_ID));
    m_Context->ReadHeader();
}

void RegionHandler::Unload() {
    if (!m_Context) return;
    if (m_Context->IsModified())
        m_Context->SaveHeader();
    delete m_Context;
    m_Context = nullptr;
}

bool RegionHandler::HasChunk(ChunkCoordinates coords) const {
    if (!m_Context) return false;
    uint64_t dummy;
    return m_Context->FindChunk(coords.GetKey(), dummy);
}

bool RegionHandler::WriteChunk(Chunk& chunk) {
    if (!m_Context) {
        LOG_ERROR("[RegionHandler] WriteChunk called before Load() (id=%" PRIu64 ").", m_ID);
        return false;
    }
    return m_Context->WriteChunk(chunk);
}

std::unique_ptr<Chunk> RegionHandler::ReadChunk(ChunkCoordinates coords) const {
    if (!m_Context) {
        LOG_ERROR("[RegionHandler] ReadChunk called before Load() (id=%" PRIu64 ").", m_ID);
        return nullptr;
    }
    return m_Context->ReadChunk(coords.GetKey());
}

uint64_t RegionHandler::MakeID(int32_t regionX, int32_t regionZ) noexcept {
    return (static_cast<uint64_t>(static_cast<uint32_t>(regionZ)) << 32)
         |  static_cast<uint64_t>(static_cast<uint32_t>(regionX));
}

void RegionHandler::DecodeID(uint64_t id, int32_t& outX, int32_t& outZ) noexcept {
    outZ = static_cast<int32_t>(id >> 32);
    outX = static_cast<int32_t>(id & 0xFFFFFFFFu);
}

ChunkCoordinates RegionHandler::ChunkToRegion(ChunkCoordinates chunk) noexcept {
    // Arithmetic (truncating) division rounds toward zero; floor division always rounds down.
    // For negative a with non-zero remainder, floor = truncation - 1.
    auto floorDiv = [](int32_t a, int32_t b) -> int32_t {
        return a / b - (a % b != 0 && (a ^ b) < 0 ? 1 : 0);
    };
    return ChunkCoordinates(floorDiv(chunk.X, REGION_SIZE),
                            floorDiv(chunk.Z, REGION_SIZE));
}

std::string RegionHandler::HeadPath(const std::string& regDir, uint64_t id) {
    int32_t rx, rz;
    DecodeID(id, rx, rz);
    return regDir + "/head_"
         + std::format("{:08x}_{:08x}", static_cast<uint32_t>(rx), static_cast<uint32_t>(rz))
         + ".bin";
}

std::string RegionHandler::DataPath(const std::string& regDir, uint64_t id) {
    int32_t rx, rz;
    DecodeID(id, rx, rz);
    return regDir + "/reg_"
         + std::format("{:08x}_{:08x}", static_cast<uint32_t>(rx), static_cast<uint32_t>(rz))
         + ".bin";
}

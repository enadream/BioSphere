#pragma once
#include <cstdint>
#include <cstddef>

// -----------------------------------------------------------------------------
// SaveType - encoded in the top 8 bits of every FieldID.
// GetSaveType(id) is a single >> 24, no lookup needed.
// Max 255 categories. Never reorder - top bits are on disk.
// -----------------------------------------------------------------------------
enum class SaveType : uint8_t {
    None   = 0,
    Player = 1,   // Encrypted
    World  = 2,   // Plain binary
    Map    = 3,   // Compressed
    Settings = 4,
};

// Extract SaveType from any FieldID at compile time or runtime
constexpr SaveType GetSaveType(uint32_t id) noexcept {
    return static_cast<SaveType>(id >> 24);
}

// FNV-1a 32-bit compile-time hash (bottom 24 bits used)
constexpr uint32_t ConstHash(const char* str, uint32_t hash = 2166136261u) noexcept {
    return *str ? ConstHash(str + 1, (hash ^ static_cast<uint8_t>(*str)) * 16777619u) : hash;
}

// -----------------------------------------------------------------------------
// MakeFieldID - top 8 bits = SaveType category, bottom 24 bits = name hash.
// Two fields in different categories can NEVER collide.
// Two fields in the same category collide with 1/16M chance -> caught by static_assert below.
// -----------------------------------------------------------------------------
constexpr uint32_t MakeFieldID(SaveType type, const char* name) noexcept {
    return (static_cast<uint32_t>(type) << 24) | (ConstHash(name) & 0x00FFFFFFu);
}

// -----------------------------------------------------------------------------
// FIELD REGISTRY
//     NEVER rename or remove string literals - the hash IS the on-disk ID.
//     Adding new entries is always safe.
//     Renaming = corrupt existing saves.
// -----------------------------------------------------------------------------

namespace FieldID {

    namespace Player {
        constexpr uint32_t Position  = MakeFieldID(SaveType::Player, "position");
        constexpr uint32_t Rotation  = MakeFieldID(SaveType::Player, "rotation");
        constexpr uint32_t Health    = MakeFieldID(SaveType::Player, "health");
        constexpr uint32_t Hunger    = MakeFieldID(SaveType::Player, "hunger");
        constexpr uint32_t Name      = MakeFieldID(SaveType::Player, "name");
        constexpr uint32_t Inventory = MakeFieldID(SaveType::Player, "inventory");
    }

    namespace Settings {
        
    }

    namespace World {
        constexpr uint32_t Seed      = MakeFieldID(SaveType::World, "seed");
        constexpr uint32_t Name      = MakeFieldID(SaveType::World, "name");
        constexpr uint32_t GameTick  = MakeFieldID(SaveType::World, "game_tick");
    }

    namespace Map {
        constexpr uint32_t ChunkData = MakeFieldID(SaveType::Map, "chunk_data");
        constexpr uint32_t ChunkPos  = MakeFieldID(SaveType::Map, "chunk_pos");
    }

} // namespace FieldID



// -----------------------------------------------------------------------------
// COMPILE-TIME UNIQUENESS CHECK
// Every ID must be listed here.
// O(n^2) constexpr check - runs only at compile time, zero runtime cost.
// If you get a static_assert failure: two field name strings hashed to the same
// bottom 24 bits within the same SaveType. Rename one string to fix it.
// -----------------------------------------------------------------------------
namespace {
    constexpr uint32_t ALL_FIELD_IDS[] = {
        // Player
        FieldID::Player::Position,
        FieldID::Player::Rotation,
        FieldID::Player::Health,
        FieldID::Player::Hunger,
        FieldID::Player::Name,
        FieldID::Player::Inventory,
        // World
        FieldID::World::Seed,
        FieldID::World::Name,
        FieldID::World::GameTick,
        // Map
        FieldID::Map::ChunkData,
        FieldID::Map::ChunkPos,
    };

    // Returns the colliding ID if one is found, 0 if all unique.
    // 0 is safe as a sentinel because MakeFieldID always sets the top 8 bits
    // to a non-zero SaveType, so a real ID can never be 0.
    constexpr uint32_t FindCollision(const uint32_t* arr, size_t n) noexcept {
        for (size_t i = 0; i < n; ++i)
            for (size_t j = i + 1; j < n; ++j)
                if (arr[i] == arr[j]) {
                    return arr[i]; // both values are identical, one is enough
                }
        return 0;
    }

    constexpr uint32_t FIELD_ID_COLLISION = FindCollision(
        ALL_FIELD_IDS, sizeof(ALL_FIELD_IDS) / sizeof(ALL_FIELD_IDS[0])
    );

    static_assert(FIELD_ID_COLLISION == 0,
        "FieldID collision detected! "
        "Check the value of FIELD_ID_COLLISION in the compiler output to identify "
        "which hash collided, then rename one of the matching strings in field_id.hpp."
    );
} // anonymous namespace
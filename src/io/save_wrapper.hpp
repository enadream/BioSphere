#pragma once

#include "io/wrapper_handle.hpp"
#include "io/field_id.hpp"
#include "core/log.hpp"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <type_traits>

// Forward declaration - full include would be circular
class SaveManager;

// Registration helpers - defined in save_wrapper.cpp to avoid circular include
namespace detail {
    void RegisterWrapper  (IWrapperHandle* handle);
    void UnregisterWrapper(uint32_t fieldId);
}

// SaveWrapperBase - two specializations:
//   is_class<T> == true  ->  inherit from T  ->  m_Position.x works via '.'
//   is_class<T> == false ->  store T + implicit cast operators -> m_Health = 5.0f works

template<typename Type, bool IsClass = std::is_class_v<Type>>
class SaveWrapperBase : public Type { //  Class path: inherit T directly
    static_assert(std::is_trivially_copyable_v<Type>,
        "SaveWrapper: Class type Type must be trivially copyable (e.g. glm::vec3, glm::quat, plain structs).");
public:
    using Type::Type; // Inherit T's constructors (e.g. glm::vec3(x,y,z))
    SaveWrapperBase() = default;
    SaveWrapperBase(const Type& v) : Type(v) {}

    // Assignment from T lets you do: m_Position = glm::vec3(1,2,3);
    SaveWrapperBase& operator=(const Type& v) { static_cast<Type&>(*this) = v;            return *this; }
    SaveWrapperBase& operator=(Type&& v)      { static_cast<Type&>(*this) = std::move(v); return *this; }

    // Implicit conversion back to T for passing to functions expecting T
    operator Type&()             { return static_cast<Type&>(*this); }
    operator const Type&() const { return static_cast<const Type&>(*this); }

    Type&       Value()       { return static_cast<Type&>(*this); }
    const Type& Value() const { return static_cast<const Type&>(*this); }
};

// For primitive types
template<typename Type>
class SaveWrapperBase<Type, false> { //  Primitive path: float, int, bool, enum...
    static_assert(std::is_trivially_copyable_v<Type>,
        "SaveWrapper: Primitive type Type must be trivially copyable.");
public:
    SaveWrapperBase() = default;
    SaveWrapperBase(const Type& v) : m_Value(v) {}

    // Transparent assignment: m_Health = 100.0f;
    SaveWrapperBase& operator=(const Type& v) { m_Value = v; return *this; }

    // Transparent read: float hp = m_Health; or PassToFunction(m_Health);
    operator Type&()             { return m_Value; }
    operator const Type&() const { return m_Value; }

    // Arithmetic passthrough (+=, -=, etc. work because of implicit cast above)
    Type&       Value()       { return m_Value; }
    const Type& Value() const { return m_Value; }

private:
    Type m_Value{};
};

// SaveWrapper<T, ID>
// Transparent wrapper for trivially copyable types (POD structs, glm types,
// primitives). Automatically registers with SaveManager on construction.
template <typename Type, uint32_t ID>
class SaveWrapper : public SaveWrapperBase<Type>, public IWrapperHandle {
public:
    static constexpr uint32_t FieldID  = ID;
    static constexpr SaveType SType = GetSaveType(ID);
    using ValueType = Type;
    using Base      = SaveWrapperBase<Type>;

    SaveWrapper() { detail::RegisterWrapper(this); }
    explicit SaveWrapper(const Type& defaultVal) : Base(defaultVal) { detail::RegisterWrapper(this); }
    ~SaveWrapper() override { detail::UnregisterWrapper(ID); }

    // Deleted - would cause double-registration or dangling pointer in registry
    SaveWrapper(const SaveWrapper&)            = delete;
    SaveWrapper(SaveWrapper&&)                 = delete;
    SaveWrapper& operator=(const SaveWrapper&) = delete;
    SaveWrapper& operator=(SaveWrapper&&)      = delete;

    // Keep transparent assignment from T (not from SaveWrapper)

    using Base::Base;               // Inherit base constructors
    using Base::operator=;          // Inherit assignment from T
    using Base::operator Type&;     // Inherit implicit casts
    using Base::operator const Type&;

    // -- IWrapperHandle --------------------------------------------------------
    uint32_t    GetFieldID() const override { return ID; }
    const void* RawPtr()     const override { return &this->Value(); }
    size_t      RawSize()    const override { return sizeof(Type); }
    void LoadRaw(const void* src, size_t size) override {
        if (size != sizeof(Type)) {
            LOG_WARN("[SaveWrapper] Size mismatch on field 0x%X: expected %zu, got %zu", ID, sizeof(Type), size);
            return;
        }
        std::memcpy(&this->Value(), src, sizeof(Type));
    }
};


// SaveWrapperString<ID>  - transparent std::string
template<uint32_t ID>
class SaveWrapperString : public std::string, public IWrapperHandle {
public:
    static constexpr uint32_t FieldID = ID;
    static constexpr SaveType Stype    = GetSaveType(ID);
    using ValueType = std::string;

    SaveWrapperString()  { detail::RegisterWrapper(this);  }
    explicit SaveWrapperString(const std::string& def) : std::string(def) {
        detail::RegisterWrapper(this);
    }
    ~SaveWrapperString() override { detail::UnregisterWrapper(ID); }

    // Deleted - would cause double-registration or dangling pointer in registry
    SaveWrapperString(const SaveWrapperString&)            = delete;
    SaveWrapperString(SaveWrapperString&&)                 = delete;
    SaveWrapperString& operator=(const SaveWrapperString&) = delete;
    SaveWrapperString& operator=(SaveWrapperString&&)      = delete;

    using std::string::string;   // All std::string constructors
    using std::string::operator=;

    
    // -- IWrapperHandle --------------------------------------------------------
    uint32_t    GetFieldID() const override { return ID;          }
    const void* RawPtr()     const override { return this->data(); }
    size_t      RawSize()    const override { return this->size(); }

    void LoadRaw(const void* src, size_t size) override {
        this->assign(reinterpret_cast<const char*>(src), size);
    }
};

// SaveWrapperVector<T, ID>  - std::vector<T> with transparent access
//
// T must be trivially copyable (plain structs, POD types).
// The entire vector is serialized as a raw contiguous byte block:
//   [ ElementCount : uint32 | sizeof(T)*N bytes ]

template<typename Type, uint32_t ID>
class SaveWrapperVector : public std::vector<Type>, public IWrapperHandle {
    static_assert(std::is_trivially_copyable_v<Type>,
        "SaveWrapperVector: T must be trivially copyable. "
        "Nested containers or heap-owning types cannot be bulk-serialized safely.");
public:
    static constexpr uint32_t FieldID   = ID;
    static constexpr SaveType SType     = GetSaveType(ID);
    using ElementType                   = Type;
    using ValueType                     = std::vector<Type>;

    SaveWrapperVector()  { detail::RegisterWrapper(this);  }
    ~SaveWrapperVector() override { detail::UnregisterWrapper(ID); }

    // Deleted - would cause double-registration or dangling pointer in registry
    SaveWrapperVector(const SaveWrapperVector&)            = delete;
    SaveWrapperVector(SaveWrapperVector&&)                 = delete;
    SaveWrapperVector& operator=(const SaveWrapperVector&) = delete;
    SaveWrapperVector& operator=(SaveWrapperVector&&)      = delete;

    using std::vector<Type>::vector;        // All std::vector constructors
    using std::vector<Type>::operator=;

    // -- IWrapperHandle --------------------------------------------------------
    uint32_t    GetFieldID() const override { return ID;                          }
    const void* RawPtr()     const override { return this->data();                }
    size_t      RawSize()    const override { return this->size() * sizeof(Type); }
    void LoadRaw(const void* src, size_t byteSize) override {
        if (byteSize % sizeof(Type) != 0) {
            LOG_WARN("[SaveWrapperBlob] Field 0x%X: byte size %zu not a multiple of element size %zu - "
                "skipping, corrupt data?", ID, byteSize, sizeof(Type));
            return;
        }
        const size_t count = byteSize / sizeof(Type);
        this->resize(count);
        std::memcpy(this->data(), src, byteSize);
    }
};
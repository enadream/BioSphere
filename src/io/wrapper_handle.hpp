#pragma once
#include <cstdint>
#include <cstddef>

// -----------------------------------------------------------------------------
// IWrapperHandle
// Type-erased interface so SaveManager can read/write any SaveWrapper<T,ID>
// without knowing T. Lives in its own header to break the circular dependency:
//   save_wrapper.hpp -> save_manager.hpp -> wrapper_handle.hpp
// -----------------------------------------------------------------------------
class IWrapperHandle {
public:
    virtual ~IWrapperHandle() = default;

    virtual uint32_t    GetFieldID() const                        = 0;
    virtual const void* RawPtr()     const                        = 0;
    virtual size_t      RawSize()    const                        = 0;
    virtual void        LoadRaw(const void* src, size_t size)     = 0;
};
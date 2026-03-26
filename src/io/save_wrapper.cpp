#include "io/save_wrapper.hpp"
#include "io/save_manager.hpp" // full include lives here, not in the header

namespace detail {
    void RegisterWrapper(IWrapperHandle* handle) {
        SaveManager::Get().Register(handle);
    }
    void UnregisterWrapper(uint32_t fieldId) {
        SaveManager::Get().Unregister(fieldId);
    }
}
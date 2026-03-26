#pragma once

#include <cstdint>
#include <string>
#include <functional>

// Helper macro for bit masking
#define BIT(x) (1 << x) 

enum class EventType : uint32_t {
    None = 0,
    WindowClose, WindowResize, WindowFocus, WindowLostFocus,
    KeyPressed, KeyReleased, KeyTyped,
    MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
};

enum EventCategory : uint32_t {
    None = 0,
    EventCategoryApplication = BIT(0),
    EventCategoryInput       = BIT(1),
    EventCategoryKeyboard    = BIT(2),
    EventCategoryMouse       = BIT(3),
    EventCategoryMouseButton = BIT(4)
};

// Macros to automate virtual function overrides
#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
                               virtual EventType GetEventType() const override { return GetStaticType(); }\
                               virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }


class Event {
public:
    virtual ~Event() = default;

    virtual EventType GetEventType() const = 0;
    virtual const char* GetName() const = 0;
    virtual int GetCategoryFlags() const = 0;

    // Debug helper
    virtual std::string ToString() const { return GetName(); }

    // Check if event belongs to a specific category (Bitwise check)
    bool IsInCategory(EventCategory category) {
        return GetCategoryFlags() & category;
    }

public:
    bool Handled = false;
};

// Dispatcher: Routes events to specific functions based on their type
class EventDispatcher {
    template<typename T>
    using EventFn = std::function<bool(T&)>;

public:
    EventDispatcher(Event& event)
        : m_Event(event) {}

    template<typename T>
    bool Dispatch(EventFn<T> func) {
        // Check if the dynamic event type matches the template type
        if (m_Event.GetEventType() == T::GetStaticType()) {
            // Cast and call the function
            m_Event.Handled = func(*(T*)&m_Event);
            return true;
        }
        return false;
    }

private:
    Event& m_Event;
};

// Allow "cout << event"
inline std::ostream& operator<<(std::ostream& os, const Event& e) {
    return os << e.ToString();
}





#pragma once

#include "events/event.hpp"
#include <sstream>


class KeyEvent : public Event {
public:
    inline int GetKeyCode() const { return m_KeyCode; }

    // Sets the event category for all derived keyboard events.
    // Keyboard events are part of both the Keyboard and general Input categories.
    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

protected:
    // Protected constructor ensures this base class is not instantiated directly.
    KeyEvent(int keycode)
        : m_KeyCode(keycode) {}

    int m_KeyCode;
};

class KeyPressedEvent : public KeyEvent {
public:
    KeyPressedEvent(int keycode, bool isRepeat = false)
			: KeyEvent(keycode), m_IsRepeat(isRepeat) {}

    bool IsRepeat() const { return m_IsRepeat; }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "KeyPressedEvent: " << m_KeyCode << " (repeat = " << m_IsRepeat << ")";
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyPressed)

private:
    bool m_IsRepeat;
};

class KeyReleasedEvent : public KeyEvent {
public:
    KeyReleasedEvent(int keycode)
        : KeyEvent(keycode) {}

    std::string ToString() const override {
        std::stringstream ss;
        ss << "KeyReleasedEvent: " << m_KeyCode;
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyReleased)
};


class KeyTypedEvent : public KeyEvent {
public:
    KeyTypedEvent(int keycode)
        : KeyEvent(keycode) {}

    std::string ToString() const override {
        std::stringstream ss;
        ss << "KeyTypedEvent: " << m_KeyCode;
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyTyped)
};
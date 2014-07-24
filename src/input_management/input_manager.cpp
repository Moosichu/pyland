#include <iostream>
#include <functional>
#include <queue>
#include <set>

extern "C" {
#include <SDL2/SDL.h>
}

#include "input_manager.hpp"
#include "game_window.hpp"
#include "keyboard_input_event.hpp"
#include "lifeline.hpp"
#include "lifeline_controller.hpp"



#ifdef DEBUG
#define GAME_WINDOW_DEBUG
#endif



InputManager::InputManager(GameWindow* window):
    window(window),

    down_keys(),
    pressed_keys(),
    typed_keys(),
    released_keys(),
    
    mouse_start(0, 0, MouseState::ButtonMask::NONE),
    mouse_from(0, 0, MouseState::ButtonMask::NONE),
    mouse_to(0, 0, MouseState::ButtonMask::NONE),
    
    key_events(),
    mouse_events(),

    callback_controller() {
}


InputManager::~InputManager() {
    callback_controller.disable();
}


void InputManager::clean() {
    pressed_keys.clear();
    typed_keys.clear();
    released_keys.clear();
}


void InputManager::handle_event(SDL_Event* event) {
    int button, buttons, button_find;
    switch (event->type) {
    case SDL_KEYDOWN:
        if (down_keys.count(event->key.keysym.scancode) == 0) {
            down_keys.insert(event->key.keysym.scancode);
            pressed_keys.insert(event->key.keysym.scancode);
            key_events.push(KeyboardInputEvent(this, event->key.keysym.scancode, true, true, true));
        }
        else {
            key_events.push(KeyboardInputEvent(this, event->key.keysym.scancode, true, false, true));
        }
        typed_keys.insert(event->key.keysym.scancode);
        break;
    case SDL_KEYUP:
        key_events.push(KeyboardInputEvent(this, event->key.keysym.scancode, false, true, false));
        down_keys.erase(event->key.keysym.scancode);
        released_keys.insert(event->key.keysym.scancode);
        break;
    case SDL_MOUSEBUTTONDOWN:
        mouse_from = mouse_to;
        mouse_to = MouseState(event->button.x,
                              event->button.y,
                              mouse_from.buttons | (1 << (event->button.button - 1)));
        mouse_events.push(MouseInputEvent(this, mouse_start, mouse_from, mouse_to, event->button.button));
        mouse_start = mouse_to;
        break;
    case SDL_MOUSEBUTTONUP:
        mouse_from = mouse_to;
        mouse_to = MouseState(event->button.x,
                              event->button.y,
                              mouse_from.buttons & ~(1 << (event->button.button - 1)));
        mouse_events.push(MouseInputEvent(this, mouse_start, mouse_from, mouse_to, event->button.button));
        mouse_start = mouse_to;
        break;
    case SDL_MOUSEMOTION:
        mouse_from = mouse_to;
        mouse_to = MouseState(event->motion.x, event->motion.y, mouse_from.buttons);
        button = 0;
        for (buttons = mouse_to.buttons, button_find = 1;
             buttons != 0;
             buttons >>= 1, button_find++) {
            if ((buttons & 1) == 1) {
                button = button_find;
                break;
            }
        }
        mouse_events.push(MouseInputEvent(this, mouse_start, mouse_from, mouse_to, button));
        break;
    }
}


void InputManager::run_callbacks() {
    while (!key_events.empty()) {
        KeyboardInputEvent& event = key_events.front();
        keyboard_callbacks.broadcast(event);
        if (event.down) {
            if (event.changed) {
                key_press_callbacks.broadcast(event);
            }
            key_down_callbacks.broadcast(event);
            if (event.typed) {
                key_type_callbacks.broadcast(event);
            }
        }
        else {
            key_release_callbacks.broadcast(event);
        }
        key_events.pop();
    }
    for (int key : down_keys) {
        // Don't duplicate events
        if (pressed_keys.count(key) == 0 && typed_keys.count(key) == 0) {
            KeyboardInputEvent event(this, key, true, false, false);
            keyboard_callbacks.broadcast(event);
            key_down_callbacks.broadcast(event);
        }
    }
    
    while (!mouse_events.empty()) {
        MouseInputEvent& event = mouse_events.front();
        mouse_callbacks.broadcast(event);
        mouse_events.pop();
    }
}


bool InputManager::is_scan_down(int key) {
    return (down_keys.count(key) == 1);
}


bool InputManager::is_key_down(int key) {
    return (down_keys.count(SDL_GetScancodeFromKey(key)) == 1);
}


bool InputManager::is_scan_pressed(int key) {
    return (pressed_keys.count(key) == 1);
}


bool InputManager::is_key_pressed(int key) {
    return (pressed_keys.count(SDL_GetScancodeFromKey(key)) == 1);
}


bool InputManager::is_scan_released(int key) {
    return (released_keys.count(key) == 1);
}


bool InputManager::is_key_released(int key) {
    return (released_keys.count(SDL_GetScancodeFromKey(key)) == 1);
}


bool InputManager::is_mouse_down(int button) {
    return mouse_to.is_down(button);
}


bool InputManager::is_mouse_pressed(int button) {
    return mouse_to.is_down(button) && !mouse_from.is_down(button);
}


bool InputManager::is_mouse_released(int button) {
    return !mouse_to.is_down(button) && mouse_from.is_down(button);
}


std::pair<int,int> InputManager::get_mouse_pixels() {
    return std::pair<int,int>(mouse_to.x, mouse_to.y);
}


std::pair<float,float> InputManager::get_mouse_ratio() {
    return window->get_ratio_from_pixels(std::pair<int,int>(mouse_to.x, mouse_to.y));
}


void InputManager::register_keyboard_handler(Callback<void, KeyboardInputEvent> callback) {
    keyboard_callbacks.register_callback(callback);
}

Lifeline InputManager::register_keyboard_handler(std::function<void(KeyboardInputEvent)> func) {
    Callback<void, KeyboardInputEvent> callback(func);
    keyboard_callbacks.register_callback(callback);
    return Lifeline([this, callback] () {
            keyboard_callbacks.unregister_callback(callback);
        },
        callback_controller);
}


void InputManager::register_key_press_handler(Callback<void, KeyboardInputEvent> callback) {
    key_press_callbacks.register_callback(callback);
}

Lifeline InputManager::register_key_press_handler(std::function<void(KeyboardInputEvent)> func) {
    Callback<void, KeyboardInputEvent> callback(func);
    key_press_callbacks.register_callback(callback);
    return Lifeline([this, callback] () {
            key_press_callbacks.unregister_callback(callback);
        },
        callback_controller);
}


void InputManager::register_key_type_handler(Callback<void, KeyboardInputEvent> callback) {
    key_type_callbacks.register_callback(callback);
}

Lifeline InputManager::register_key_type_handler(std::function<void(KeyboardInputEvent)> func) {
    Callback<void, KeyboardInputEvent> callback(func);
    key_type_callbacks.register_callback(callback);
    return Lifeline([this, callback] () {
            key_type_callbacks.unregister_callback(callback);
        },
        callback_controller);
}


void InputManager::register_key_down_handler(Callback<void, KeyboardInputEvent> callback) {
    key_down_callbacks.register_callback(callback);
}

Lifeline InputManager::register_key_down_handler(std::function<void(KeyboardInputEvent)> func) {
    Callback<void, KeyboardInputEvent> callback(func);
    key_down_callbacks.register_callback(callback);
    return Lifeline([this, callback] () {
            key_down_callbacks.unregister_callback(callback);
        },
        callback_controller);
}


void InputManager::register_key_release_handler(Callback<void, KeyboardInputEvent> callback) {
    key_release_callbacks.register_callback(callback);
}

Lifeline InputManager::register_key_release_handler(std::function<void(KeyboardInputEvent)> func) {
    Callback<void, KeyboardInputEvent> callback(func);
    key_release_callbacks.register_callback(callback);
    return Lifeline([this, callback] () {
            key_release_callbacks.unregister_callback(callback);
        },
        callback_controller);
}


void InputManager::register_mouse_handler(Callback<void, MouseInputEvent> callback) {
    mouse_callbacks.register_callback(callback);
}

Lifeline InputManager::register_mouse_handler(std::function<void(MouseInputEvent)> func) {
    Callback<void, MouseInputEvent> callback(func);
    mouse_callbacks.register_callback(callback);
    return Lifeline([this, callback] () {
            mouse_callbacks.unregister_callback(callback);
        },
        callback_controller);
}


#pragma once

#include "defines.h"

typedef struct event_context {
    //128 bytes

    union {
        i64 i64[2];
        u64 u64[2];
        f64 f64[2];

        i32 i32[4];
        u32 u32[4];
        f32 f32[4];

        i16 i16[8];
        u16 u16[8];

        i8 i8[16];
        u8 u8[16];

        char c[16];

    } data;
} event_context;

//should return TRUE if the event was handled and should not be propagated further
typedef b8 (*PFN_on_event)(i16 code, void* sender, void* listener_inst, event_context data);

b8 event_initialize();

void event_shutdown();

//FOLLOWING METHODS ARE EXPORTED

//Register to listen for when events are sent with the pvovided code.

KAPI b8 event_register(u16 code, void* listener, PFN_on_event on_event);

//Unregister to listen for when events are sent with the pvovided code.

KAPI b8 event_unregister(u16 code, void* listener, PFN_on_event on_event);

//Fire an event to listeners of the given code.
KAPI b8 event_fire(u16 code, void* sender, event_context data);

typedef enum system_event_code {
    // Shuts the application down on the next frame.
    EVENT_CODE_APPLICATION_QUIT = 0x01,

    // Keyboard key pressed.
    /* Context usage:
     * u16 key_code = data.data.u16[0];
     */
    EVENT_CODE_KEY_PRESSED = 0x02,

    // Keyboard key released.
    /* Context usage:
     * u16 key_code = data.data.u16[0];
     */
    EVENT_CODE_KEY_RELEASED = 0x03,

    // Mouse button pressed.
    /* Context usage:
     * u16 button = data.data.u16[0];
     */
    EVENT_CODE_BUTTON_PRESSED = 0x04,

    // Mouse button released.
    /* Context usage:
     * u16 button = data.data.u16[0];
     */
    EVENT_CODE_BUTTON_RELEASED = 0x05,

    // Mouse moved.
    /* Context usage:
     * u16 x = data.data.u16[0];
     * u16 y = data.data.u16[1];
     */
    EVENT_CODE_MOUSE_MOVED = 0x06,

    // Mouse moved.
    /* Context usage:
     * u8 z_delta = data.data.u8[0];
     */
    EVENT_CODE_MOUSE_WHEEL = 0x07,

    // Resized/resolution changed from the OS.
    /* Context usage:
     * u16 width = data.data.u16[0];
     * u16 height = data.data.u16[1];
     */
    EVENT_CODE_RESIZED = 0x08,

    MAX_EVENT_CODE = 0xFF
} system_event_code;


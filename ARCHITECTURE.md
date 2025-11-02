# Ignis Engine - Technical Architecture Documentation

## Table of Contents
1. [Overview](#overview)
2. [Project Structure](#project-structure)
3. [Architectural Patterns](#architectural-patterns)
4. [Core Subsystems](#core-subsystems)
5. [Platform Abstraction Layer](#platform-abstraction-layer)
6. [Event System](#event-system)
7. [Input System](#input-system)
8. [Memory Management](#memory-management)
9. [Renderer Architecture](#renderer-architecture)
10. [Data Structures](#data-structures)
11. [Design Decisions](#design-decisions)

---

## Overview

The Ignis Engine is a cross-platform 3D game engine built in pure C with a focus on modularity, platform independence, and minimal external dependencies. The architecture follows a **layered, modular design** with clear separation of concerns.

### Architecture Layers

```
┌─────────────────────────────┐
│     Game Layer              │  ← testbed/src/game.c
├─────────────────────────────┤
│  Application/Renderer       │  ← application.c, renderer_frontend.h
├─────────────────────────────┤
│  Core Subsystems            │  ← event, input, clock, logger, kmemory
├─────────────────────────────┤
│  Platform Abstraction       │  ← platform.h (interface)
├─────────────────────────────┤
│  Platform Implementations   │  ← platform_win32.c, platform_linux.c
└─────────────────────────────┘
```

---

## Project Structure

```
D:\Projects\Ignis\code\
├── engine/
│   └── src/
│       ├── containers/          # Data structure implementations
│       │   ├── darray.c/h      # Dynamic array (resizable vector)
│       ├── core/                # Core engine subsystems
│       │   ├── application.c/h  # Main application loop and lifecycle
│       │   ├── event.c/h        # Event system (pub/sub pattern)
│       │   ├── input.c/h        # Input handling (keyboard/mouse)
│       │   ├── clock.c/h        # Frame timing and delta time
│       │   ├── kmemory.c/h      # Tagged memory allocation system
│       │   ├── logger.c/h       # Logging subsystem
│       │   ├── kstring.c/h      # String utilities
│       │   └── asserts.h        # Assertion macros
│       ├── platform/            # Platform abstraction layer
│       │   ├── platform.h       # Platform interface (abstract)
│       │   ├── platform_win32.c # Windows-specific implementation
│       │   └── platform_linux.c # Linux-specific implementation (X11/XCB)
│       ├── renderer/            # Rendering subsystem
│       │   ├── renderer_frontend.h   # High-level renderer API
│       │   ├── renderer_backend.h    # Backend abstraction
│       │   ├── renderer_types.inl    # Renderer type definitions
│       │   └── vulkan/          # Vulkan backend (planned)
│       ├── defines.h            # Global type definitions and platform detection
│       ├── game_types.h         # Game structure interfaces
│       └── entry.h              # Main entry point template
└── testbed/
    └── src/
        ├── game.c/h            # Sample game implementation
        └── entry.c             # Game instantiation
```

---

## Architectural Patterns

### 1. Layered Architecture
The engine enforces strict **downward-only dependencies**:
- Game layer depends on Application layer
- Application depends on Core subsystems
- Core subsystems depend on Platform layer
- Platform layer depends only on OS APIs

Benefits: Testability, platform independence, clear module boundaries

### 2. Interface Segregation
Each subsystem exports a clean, minimal interface:
- **Platform operations**: `platform.h` (abstract interface)
- **Event system**: `event.h` (pub/sub callbacks)
- **Renderer**: `renderer_frontend.h` and `renderer_backend.h` (strategy pattern)

### 3. Factory Pattern
- `renderer_backend_create(type)` instantiates different backends (Vulkan, OpenGL, DirectX)
- Platform-specific implementations selected at compile-time via `#if KPLATFORM_*` guards

### 4. Observer Pattern (Event System)
- Event system uses registered callbacks with listener objects
- Decouples event producers from consumers
- Events propagate until first handler returns `TRUE`

### 5. Opaque Pointers
- `platform_state` contains `void* internal_state`
- Hides OS-specific details (HWND, xcb_connection_t) from engine code
- True platform abstraction without conditional compilation above platform layer

### 6. Plugin Architecture (Game Interface)
- Game doesn't inherit from engine classes
- Instead, provides callback functions via function pointers
- Pure C - no virtual methods or inheritance
- Enables game DLL hot-reloading

---

## Core Subsystems

### Application Lifecycle

**File**: `engine/src/core/application.c/h`

**Responsibilities**:
- Main engine initialization and shutdown
- Game loop orchestration
- Frame timing and rate limiting

**State Structure**:
```c
typedef struct application_state {
    game* game_inst;           // Game instance pointer
    b8 is_running;            // Loop control flag
    b8 is_suspended;          // Pause flag
    platform_state platform;  // Platform state handle
    i16 width, height;        // Window dimensions
    clock clock;              // Frame timer
    f64 last_time;            // Previous frame time
} application_state;
```

**Initialization Sequence**:
```
1. initialize_memory()
2. create_game() [game-provided callback]
3. application_create()
   3a. initialize_logging()
   3b. input_initialize()
   3c. event_initialize()
   3d. platform_startup()  [creates window]
   3e. game_inst->initialize()
4. application_run()
```

**Main Loop** (`application_run()`):
```c
while (is_running) {
    // 1. Poll OS for events
    platform_pump_messages();

    // 2. Update and render if not suspended
    if (!is_suspended) {
        clock_update(&state.clock);
        f64 current_time = state.clock.elapsed;
        f64 delta = current_time - state.last_time;

        game_inst->update(game_inst, delta);
        game_inst->render(game_inst, delta);

        // Frame rate limiting (target 60 FPS)
        f64 frame_time = platform_get_absolute_time() - current_time;
        if (frame_time < target_frame_seconds) {
            platform_sleep((target_frame_seconds - frame_time) * 1000);
        }

        state.last_time = current_time;
    }

    // 3. Update input state (copy current → previous)
    input_update(delta);
}
```

**Shutdown Sequence**:
```
1. game_inst->shutdown()
2. event_shutdown()
3. input_shutdown()
4. platform_shutdown()
5. shutdown_logging()
```

### Clock System

**File**: `engine/src/core/clock.c/h`

**Purpose**: Frame timing and delta time calculations

**Structure**:
```c
typedef struct clock {
    f64 start_time;    // Absolute start time
    f64 elapsed;       // Accumulated time since start
} clock;
```

**Operations**:
- `clock_start()`: Reset elapsed to 0, capture start time
- `clock_update()`: Update elapsed using `platform_get_absolute_time()`
- `clock_stop()`: Pause clock (doesn't reset elapsed)

**Usage**:
```c
clock_update(&app_state.clock);
f64 current_time = app_state.clock.elapsed;
f64 delta = current_time - app_state.last_time;  // Frame delta in seconds
```

### Logger System

**File**: `engine/src/core/logger.c/h`

**Purpose**: Formatted logging with severity levels and color-coded output

**Log Levels**:
```c
LOG_LEVEL_FATAL (0)   // Red, critical errors
LOG_LEVEL_ERROR (1)   // Red, errors
LOG_LEVEL_WARN (2)    // Yellow, warnings
LOG_LEVEL_INFO (3)    // Green, information
LOG_LEVEL_DEBUG (4)   // Blue, debug info (disabled in release)
LOG_LEVEL_TRACE (5)   // Gray, trace (disabled in release)
```

**Macros**:
```c
KFATAL(message, ...)   // Format: "[FATAL]: message"
KERROR(message, ...)
KWARN(message, ...)
KINFO(message, ...)
KDEBUG(message, ...)   // Compiled out in KRELEASE
KTRACE(message, ...)   // Compiled out in KRELEASE
```

**Features**:
- 32KB message buffer per log call
- Variable argument support (`va_list`)
- Platform-specific color output
- Separate error stream for FATAL/ERROR levels

---

## Platform Abstraction Layer

**File**: `engine/src/platform/platform.h` (interface)

### Platform Detection

**File**: `engine/src/defines.h`

```c
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #define KPLATFORM_WINDOWS 1
#elif defined(__linux__) || defined(__gnu_linux__)
    #define KPLATFORM_LINUX 1
#elif defined(__APPLE__)
    #define KPLATFORM_APPLE 1  // Future support
#elif defined(__unix__)
    #define KPLATFORM_UNIX 1
#endif
```

### Platform Interface

**Opaque State**:
```c
typedef struct platform_state {
    void* internal_state;  // Platform-specific data
} platform_state;
```

**Core Functions**:

| Function | Purpose |
|----------|---------|
| `platform_startup()` | Create window, initialize platform |
| `platform_shutdown()` | Destroy window, cleanup |
| `platform_pump_messages()` | Poll OS events |
| `platform_allocate(size, aligned)` | Allocate memory |
| `platform_free(block, aligned)` | Free memory |
| `platform_zero_memory()` | Zero memory block |
| `platform_copy_memory()` | Copy memory block |
| `platform_set_memory()` | Set memory block to value |
| `platform_console_write()` | Write colored output |
| `platform_console_write_error()` | Write error output |
| `platform_get_absolute_time()` | High-resolution timer (seconds) |
| `platform_sleep(ms)` | Sleep thread |

### Windows Implementation

**File**: `engine/src/platform/platform_win32.c`

**Internal State**:
```c
typedef struct internal_state {
    HINSTANCE h_instance;  // Application instance handle
    HWND hwnd;             // Window handle
} internal_state;
```

**Implementation Details**:
- **Windowing**: `CreateWindowExA()` + `RegisterClassA()`
- **Message Loop**: `PeekMessageA()` with `PM_REMOVE` flag
- **Message Callback**: `win32_process_message()` handles WM_* messages
- **Input Routing**:
  - `WM_KEYDOWN/UP`, `WM_SYSKEYDOWN/UP` → `input_process_key()`
  - `WM_MOUSEMOVE` → `input_process_mouse_move()`
  - `WM_LBUTTONDOWN/UP`, `WM_RBUTTONDOWN/UP`, `WM_MBUTTONDOWN/UP` → `input_process_button()`
  - `WM_MOUSEWHEEL` → `input_process_mouse_wheel()`
- **Timing**: `QueryPerformanceCounter()` / `QueryPerformanceFrequency()`
- **Console**: `SetConsoleTextAttribute()` + `WriteConsoleA()`
- **Memory**: Standard `malloc()` / `free()`

**Key Code Translation**: Windows VK_* codes → Engine KEY_* enum

### Linux Implementation

**File**: `engine/src/platform/platform_linux.c`

**Internal State**:
```c
typedef struct internal_state {
    Display* display;                // X11 display connection
    xcb_connection_t* connection;    // XCB connection
    xcb_window_t window;             // Window ID
    xcb_screen_t* screen;            // Screen info
    xcb_atom_t wm_protocols;         // WM_PROTOCOLS atom
    xcb_atom_t wm_delete_win;        // WM_DELETE_WINDOW atom
} internal_state;
```

**Implementation Details**:
- **Windowing**: `XOpenDisplay()` + `xcb_create_window()`
- **Window Manager Protocol**: Proper WM_DELETE_WINDOW atom handling
- **Event Loop**: `xcb_poll_for_event()` polling
- **Input Routing**:
  - `XCB_KEY_PRESS/RELEASE` + XKB translation → `input_process_key()`
  - `XCB_MOTION_NOTIFY` → `input_process_mouse_move()`
  - `XCB_BUTTON_PRESS/RELEASE` → `input_process_button()` or mouse wheel
- **Timing**: `clock_gettime(CLOCK_MONOTONIC)`
- **Console**: ANSI escape sequences via `printf()`
- **Memory**: Standard `malloc()` / `free()`

**Key Code Translation**: X11 KeySym (via XKB) → Engine KEY_* enum

### Cross-Platform Comparison

| **Feature** | **Windows** | **Linux** |
|-------------|-------------|-----------|
| **Windowing API** | Win32 `CreateWindowExA()` | XCB `xcb_create_window()` |
| **Event Loop** | `PeekMessageA(PM_REMOVE)` | `xcb_poll_for_event()` |
| **Keyboard Input** | WM_KEYDOWN/UP messages | XCB_KEY_PRESS/RELEASE events |
| **Mouse Input** | WM_MOUSEMOVE, WM_*BUTTON* | XCB_MOTION_NOTIFY, XCB_BUTTON_* |
| **Key Translation** | VK_* codes (direct mapping) | KeySym via XKB library |
| **High-Res Timer** | QueryPerformanceCounter | clock_gettime(CLOCK_MONOTONIC) |
| **Console Color** | SetConsoleTextAttribute | ANSI escape sequences |
| **Mouse Wheel** | WHEEL_DELTA / 120 → ±1 | Button 4/5 → ±1 |

---

## Event System

**File**: `engine/src/core/event.c/h`

### Architecture

**Purpose**: Decoupled publish/subscribe event routing using observer pattern

**Capacity**: 16,384 event codes (16-bit event IDs: `u16`)

**Data Structures**:
```c
// Event callback signature
typedef b8 (*PFN_on_event)(
    u16 code,                   // Event code (0-65535)
    void* sender,               // Event originator
    void* listener_inst,        // Listener object
    event_context data          // 128-byte payload
);
// Returns TRUE to stop propagation, FALSE to continue

// 128-byte event payload
typedef struct event_context {
    union {
        i64 i64[2];  u64 u64[2];  f64 f64[2];
        i32 i32[4];  u32 u32[4];  f32 f32[4];
        i16 i16[8];  u16 u16[8];
        i8 i8[16];   u8 u8[16];
        char c[16];
    } data;
} event_context;

// Internal: registered event entry
typedef struct registered_event {
    void* listener;           // Listener object instance
    PFN_on_event callback;    // Callback function
} registered_event;

// Internal: event system state
typedef struct event_system_state {
    // Lookup table: event_code → darray<registered_event>
    registered_event* registered[MAX_MESSAGE_CODES];
} event_system_state;
```

### Core Functions

**Initialization**:
```c
b8 event_initialize();        // Allocate state, zero lookup table
void event_shutdown();        // Free all darrays, free state
```

**Registration**:
```c
// Subscribe listener to event code
b8 event_register(
    u16 code,                 // Event code to listen for
    void* listener,           // Listener object (this pointer)
    PFN_on_event on_event     // Callback function
);

// Unsubscribe listener from event code
b8 event_unregister(
    u16 code,
    void* listener,
    PFN_on_event on_event
);
```

**Event Firing**:
```c
// Fire event to all registered listeners
b8 event_fire(
    u16 code,                 // Event code to fire
    void* sender,             // Originator (for context)
    event_context context     // 128-byte payload
);
// Returns TRUE if any handler consumed the event (stops propagation)
```

### Predefined Event Codes

**File**: `engine/src/core/event.h`

```c
typedef enum system_event_code {
    EVENT_CODE_APPLICATION_QUIT = 0x01,

    // Keyboard events
    EVENT_CODE_KEY_PRESSED = 0x02,      // data.u16[0] = key code
    EVENT_CODE_KEY_RELEASED = 0x03,     // data.u16[0] = key code

    // Mouse button events
    EVENT_CODE_BUTTON_PRESSED = 0x04,   // data.u16[0] = button
    EVENT_CODE_BUTTON_RELEASED = 0x05,  // data.u16[0] = button

    // Mouse movement/wheel
    EVENT_CODE_MOUSE_MOVED = 0x06,      // data.u16[0]=x, data.u16[1]=y
    EVENT_CODE_MOUSE_WHEEL = 0x07,      // data.u8[0] = z_delta (±1)

    // Window events
    EVENT_CODE_RESIZED = 0x08,          // data.u16[0]=width, data.u16[1]=height

    MAX_EVENT_CODE = 0xFF               // Reserve 0x00-0xFF for engine
} system_event_code;
```

**User Event Codes**: Application can use codes 0x0100 - 0x3FFF (16,384 total)

### Event Flow Example

**Scenario**: User presses ESC key → Application quits

```
1. Platform Layer (Windows):
   WM_KEYDOWN(VK_ESCAPE) received in win32_process_message()
   ↓
2. Input System:
   input_process_key(KEY_ESCAPE, pressed=TRUE)
   - Updates keyboard_current.keys[KEY_ESCAPE] = TRUE
   - Fires event: event_fire(EVENT_CODE_KEY_PRESSED, 0, {.data.u16[0] = KEY_ESCAPE})
   ↓
3. Event System:
   - Lookup listeners for EVENT_CODE_KEY_PRESSED
   - Call application_on_key(KEY_PRESSED, 0, app_state, {KEY_ESCAPE})
   ↓
4. Application Event Handler:
   if (code == EVENT_CODE_KEY_PRESSED && context.data.u16[0] == KEY_ESCAPE) {
       event_fire(EVENT_CODE_APPLICATION_QUIT, 0, {});
       return TRUE;  // Event consumed
   }
   ↓
5. Event System:
   - Lookup listeners for EVENT_CODE_APPLICATION_QUIT
   - Call application_on_event(QUIT, 0, app_state, {})
   ↓
6. Application Quit Handler:
   state->is_running = FALSE;
   return TRUE;
   ↓
7. Application Main Loop:
   Loop detects is_running == FALSE, exits
```

### Design Benefits

1. **Decoupling**: Input system doesn't directly call application code
2. **Extensibility**: Game can register additional listeners for engine events
3. **Propagation Control**: First handler to return TRUE stops event propagation
4. **Type Safety**: 128-byte union provides flexible, zero-copy payload
5. **Performance**: Direct lookup table (O(1) listener lookup per event code)
6. **Hot-Reload Ready**: Listeners can be registered/unregistered at runtime

---

## Input System

**File**: `engine/src/core/input.c/h`

### Architecture

**Purpose**: Cross-platform keyboard and mouse input state management

**Pattern**: Double-buffered state (current frame + previous frame)

**State Structures**:
```c
// Keyboard state (256 keys)
typedef struct keyboard_state {
    b8 keys[256];  // TRUE = pressed, FALSE = released
} keyboard_state;

// Mouse state
typedef struct mouse_state {
    i16 x, y;                          // Screen coordinates
    u8 buttons[BUTTON_MAX_BUTTONS];    // 3 buttons: left, right, middle
} mouse_state;

// Input system state
typedef struct input_state {
    keyboard_state keyboard_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
} input_state;
```

### Key Codes

**Enumeration**: 256 key codes based on Windows Virtual Key codes

```c
typedef enum keys {
    // Control keys
    KEY_BACKSPACE = 0x08,
    KEY_ENTER = 0x0D,
    KEY_TAB = 0x09,
    KEY_SHIFT = 0x10,
    KEY_CONTROL = 0x11,
    KEY_PAUSE = 0x13,
    KEY_CAPITAL = 0x14,  // Caps Lock
    KEY_ESCAPE = 0x1B,
    KEY_SPACE = 0x20,

    // Arrow keys
    KEY_PRIOR = 0x21,    // Page Up
    KEY_NEXT = 0x22,     // Page Down
    KEY_END = 0x23,
    KEY_HOME = 0x24,
    KEY_LEFT = 0x25,
    KEY_UP = 0x26,
    KEY_RIGHT = 0x27,
    KEY_DOWN = 0x28,

    // Alphanumeric
    KEY_A = 0x41, KEY_B = 0x42, ..., KEY_Z = 0x5A,
    KEY_0 = 0x30, KEY_1 = 0x31, ..., KEY_9 = 0x39,

    // Numpad
    KEY_NUMPAD0 = 0x60, ..., KEY_NUMPAD9 = 0x69,
    KEY_MULTIPLY = 0x6A,  // *
    KEY_ADD = 0x6B,       // +
    KEY_SEPARATOR = 0x6C,
    KEY_SUBTRACT = 0x6D,  // -
    KEY_DECIMAL = 0x6E,   // .
    KEY_DIVIDE = 0x6F,    // /

    // Function keys
    KEY_F1 = 0x70, KEY_F2 = 0x71, ..., KEY_F24 = 0x87,

    // Lock keys
    KEY_NUMLOCK = 0x90,
    KEY_SCROLL = 0x91,

    // Shift variants
    KEY_LSHIFT = 0xA0,
    KEY_RSHIFT = 0xA1,
    KEY_LCONTROL = 0xA2,
    KEY_RCONTROL = 0xA3,
    KEY_LMENU = 0xA4,     // Left Alt
    KEY_RMENU = 0xA5,     // Right Alt

    // Special
    KEY_SEMICOLON = 0xBA,  // ;
    KEY_PLUS = 0xBB,       // =
    KEY_COMMA = 0xBC,      // ,
    KEY_MINUS = 0xBD,      // -
    KEY_PERIOD = 0xBE,     // .
    KEY_SLASH = 0xBF,      // /
    KEY_GRAVE = 0xC0,      // `

    KEYS_MAX_KEYS
} keys;
```

### Mouse Buttons

```c
typedef enum buttons {
    BUTTON_LEFT = 0,
    BUTTON_RIGHT = 1,
    BUTTON_MIDDLE = 2,
    BUTTON_MAX_BUTTONS = 3
} buttons;
```

### Core Functions

**Lifecycle**:
```c
void input_initialize();   // Zero state, called by application_create()
void input_shutdown();     // Cleanup
void input_update(f64 delta_time);  // Copy current → previous (each frame)
```

**Platform Interface** (called by platform layer):
```c
// Keyboard input processing
void input_process_key(keys key, b8 pressed);

// Mouse button processing
void input_process_button(buttons button, b8 pressed);

// Mouse movement processing
void input_process_mouse_move(i16 x, i16 y);

// Mouse wheel processing
void input_process_mouse_wheel(i8 z_delta);  // ±1
```

**Query Functions** (called by game/application):
```c
// Keyboard queries
b8 input_is_key_down(keys key);       // TRUE if currently pressed
b8 input_is_key_up(keys key);         // TRUE if currently released
b8 input_was_key_down(keys key);      // TRUE if pressed last frame
b8 input_was_key_up(keys key);        // TRUE if released last frame

// Mouse button queries
b8 input_is_button_down(buttons button);
b8 input_is_button_up(buttons button);
b8 input_was_button_down(buttons button);
b8 input_was_button_up(buttons button);

// Mouse position queries
void input_get_mouse_position(i32* x, i32* y);
void input_get_previous_mouse_position(i32* x, i32* y);
```

### Operation Flow

**Each Frame**:
```
1. Platform polls OS events (platform_pump_messages)
2. OS events trigger platform callbacks:
   - Keyboard: win32_process_message(WM_KEYDOWN) → input_process_key(key, TRUE)
   - Mouse move: win32_process_message(WM_MOUSEMOVE) → input_process_mouse_move(x, y)
   - Mouse button: win32_process_message(WM_LBUTTONDOWN) → input_process_button(BUTTON_LEFT, TRUE)

3. input_process_* functions:
   a. Update input_state.keyboard_current or mouse_current
   b. Fire appropriate event (EVENT_CODE_KEY_PRESSED, EVENT_CODE_MOUSE_MOVED, etc.)

4. Game/application queries input state:
   - input_is_key_down(KEY_W)
   - input_get_mouse_position(&x, &y)

5. End of frame: input_update(delta)
   - Copies current → previous
   - Enables "state change" detection next frame
```

**State Change Detection Examples**:
```c
// Detect "key just pressed" (edge detection)
b8 key_just_pressed = input_is_key_down(KEY_SPACE) && input_was_key_up(KEY_SPACE);

// Detect "key just released"
b8 key_just_released = input_is_key_up(KEY_SPACE) && input_was_key_down(KEY_SPACE);

// Detect "key held down"
b8 key_held = input_is_key_down(KEY_W) && input_was_key_down(KEY_W);

// Mouse movement delta
i32 prev_x, prev_y, curr_x, curr_y;
input_get_previous_mouse_position(&prev_x, &prev_y);
input_get_mouse_position(&curr_x, &curr_y);
i32 delta_x = curr_x - prev_x;
i32 delta_y = curr_y - prev_y;
```

### Platform Translation

**Windows → Engine**:
- Platform receives VK_* codes from WM_KEYDOWN/UP
- Direct mapping: `VK_ESCAPE` (0x1B) → `KEY_ESCAPE` (0x1B)
- Mouse wheel: `GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA` → ±1

**Linux → Engine**:
- Platform receives X11 KeySym via XKB library
- Translation table: `XK_Escape` → `KEY_ESCAPE` (0x1B)
- Mouse wheel: Button 4 (scroll up) → +1, Button 5 (scroll down) → -1

**Result**: Game code uses engine key codes (`KEY_*`, `BUTTON_*`) and remains platform-agnostic

---

## Memory Management

**File**: `engine/src/core/kmemory.c/h`

### Architecture

**Purpose**: Tagged memory allocation with statistics tracking

**Design**: Wrapper around platform allocator with categorized tracking

### Memory Tags

**19 Categories** (enum `memory_tag`):
```c
typedef enum memory_tag {
    MEMORY_TAG_UNKNOWN,           // Uncategorized
    MEMORY_TAG_ARRAY,             // Fixed arrays
    MEMORY_TAG_DARRAY,            // Dynamic arrays
    MEMORY_TAG_DICT,              // Dictionaries/hashmaps
    MEMORY_TAG_RING_QUEUE,        // Ring buffers
    MEMORY_TAG_BST,               // Binary search trees
    MEMORY_TAG_STRING,            // String allocations
    MEMORY_TAG_APPLICATION,       // Application-level allocations
    MEMORY_TAG_JOB,               // Job system
    MEMORY_TAG_TEXTURE,           // Texture data
    MEMORY_TAG_MATERIAL_INSTANCE, // Material instances
    MEMORY_TAG_RENDERER,          // Renderer allocations
    MEMORY_TAG_GAME,              // Game-specific allocations
    MEMORY_TAG_TRANSFORM,         // Transform components
    MEMORY_TAG_ENTITY,            // Entity objects
    MEMORY_TAG_ENTITY_NODE,       // Entity scene graph nodes
    MEMORY_TAG_SCENE,             // Scene data
    MEMORY_TAG_MAX_TAGS
} memory_tag;
```

### Statistics Tracking

**Internal State**:
```c
typedef struct memory_stats {
    u64 total_allocated;              // Total bytes allocated
    u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];  // Per-tag byte counts
} memory_stats;

static memory_stats stats;
```

### Core Functions

**Initialization**:
```c
void initialize_memory();   // Zero statistics, called before any allocations
void shutdown_memory();     // Log final memory report
```

**Allocation**:
```c
// Allocate memory with tag tracking
void* kallocate(u64 size, memory_tag tag);
// - Calls platform_allocate(size, FALSE)
// - Zeros memory via platform_zero_memory()
// - Updates stats.tagged_allocations[tag] += size
// - Updates stats.total_allocated += size
// - Returns pointer

// Free memory with tag tracking
void kfree(void* block, u64 size, memory_tag tag);
// - Updates stats.tagged_allocations[tag] -= size
// - Updates stats.total_allocated -= size
// - Calls platform_free(block, FALSE)
```

**Memory Operations**:
```c
// Zero memory block
void* kzero_memory(void* block, u64 size);
// - Delegates to platform_zero_memory()

// Copy memory block
void* kcopy_memory(void* dest, const void* source, u64 size);
// - Delegates to platform_copy_memory()

// Set memory block to value
void* kset_memory(void* dest, i32 value, u64 size);
// - Delegates to platform_set_memory()
```

**Statistics**:
```c
// Get human-readable memory usage string
char* get_memory_usage_str();
// Returns formatted string like:
// "System memory use (tagged):
//  UNKNOWN:  0 bytes
//  ARRAY:    1.25 KiB
//  DARRAY:   512 bytes
//  RENDERER: 2.50 MiB
//  GAME:     128.00 KiB
//  TOTAL:    132.26 MiB"
```

### Usage Example

```c
// Allocate texture data
u64 texture_size = width * height * 4;  // RGBA
u8* texture_data = kallocate(texture_size, MEMORY_TAG_TEXTURE);

// Use texture...

// Free texture
kfree(texture_data, texture_size, MEMORY_TAG_TEXTURE);
```

### Platform Delegation

All actual allocation delegates to platform layer:
```c
// Windows/Linux: Uses malloc/free
void* platform_allocate(u64 size, b8 aligned);
void platform_free(void* block, b8 aligned);
void* platform_zero_memory(void* block, u64 size);   // memset(block, 0, size)
void* platform_copy_memory(void* dest, const void* source, u64 size);  // memcpy
void* platform_set_memory(void* dest, i32 value, u64 size);  // memset
```

**Note**: Aligned allocation support planned but not yet implemented

### Design Benefits

1. **Leak Detection**: Compare allocation counts at shutdown
2. **Profiling**: Identify which subsystems consume most memory
3. **Debugging**: Tag-based tracking helps isolate memory issues
4. **Accounting**: Per-subsystem memory budgets
5. **Portability**: Platform layer handles OS-specific allocation

---

## Renderer Architecture

**Files**:
- `engine/src/renderer/renderer_frontend.h` (high-level API)
- `engine/src/renderer/renderer_backend.h` (backend interface)
- `engine/src/renderer/renderer_types.inl` (type definitions)

### Architecture

**Pattern**: Strategy pattern with pluggable backends

**Current Status**: Interface defined, implementations pending

### Frontend API

**File**: `renderer_frontend.h`

**Purpose**: High-level rendering interface for application/game

**Functions**:
```c
// Initialize renderer with backend type
b8 renderer_initialize(
    const char* application_name,
    struct platform_state* plat_state
);

// Shutdown renderer
void renderer_shutdown();

// Handle window resize
void renderer_on_resized(u16 width, u16 height);

// Draw frame
b8 renderer_draw_frame(render_packet* packet);
```

**Render Packet** (minimal, designed to expand):
```c
typedef struct render_packet {
    f32 delta_time;  // Frame delta time
} render_packet;
```

### Backend Interface

**File**: `renderer_backend.h`

**Purpose**: Abstract backend implementation (Vulkan, OpenGL, DirectX)

**Backend Types**:
```c
typedef enum renderer_backend_type {
    RENDERER_BACKEND_TYPE_VULKAN,   // Primary target
    RENDERER_BACKEND_TYPE_OPENGL,   // Planned
    RENDERER_BACKEND_TYPE_DIRECTX,  // Planned
} renderer_backend_type;
```

**Backend Structure** (function pointer vtable):
```c
typedef struct renderer_backend {
    struct platform_state* plat_state;  // Platform state handle

    // Backend lifecycle
    b8 (*initialize)(
        struct renderer_backend* backend,
        const char* application_name,
        struct platform_state* plat_state
    );
    void (*shutdown)(struct renderer_backend* backend);

    // Window events
    void (*resized)(
        struct renderer_backend* backend,
        u16 width,
        u16 height
    );

    // Frame rendering
    b8 (*begin_frame)(
        struct renderer_backend* backend,
        f32 delta_time
    );
    b8 (*end_frame)(
        struct renderer_backend* backend,
        f32 delta_time
    );
} renderer_backend;
```

**Factory Function**:
```c
// Create backend of specified type
b8 renderer_backend_create(
    renderer_backend_type type,
    struct platform_state* plat_state,
    renderer_backend* out_renderer_backend
);

void renderer_backend_destroy(renderer_backend* renderer_backend);
```

### Integration Flow

**Initialization** (in `application_create()`):
```c
1. renderer_initialize("My Game", &platform_state)
   ↓
2. renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, ...)
   ↓
3. backend->initialize(backend, "My Game", &platform_state)
   - Create Vulkan instance
   - Create surface from platform window
   - Select physical device
   - Create logical device, swapchain, render passes
```

**Frame Rendering** (in `application_run()` loop):
```c
1. game_inst->render(game_inst, delta_time)
   - Build render_packet with geometry, materials, lights
   ↓
2. renderer_draw_frame(&packet)
   ↓
3. backend->begin_frame(backend, delta_time)
   - Acquire swapchain image
   - Begin command buffer recording
   ↓
4. Frontend processes packet:
   - Upload uniforms
   - Bind pipelines
   - Record draw commands
   ↓
5. backend->end_frame(backend, delta_time)
   - End command buffer
   - Submit to GPU queue
   - Present swapchain image
```

### Vulkan Backend (Planned)

**Directory**: `engine/src/renderer/vulkan/`

**Planned Files**:
- `vulkan_backend.c/h`: Backend implementation
- `vulkan_device.c/h`: Device selection and creation
- `vulkan_swapchain.c/h`: Swapchain management
- `vulkan_renderpass.c/h`: Render pass creation
- `vulkan_command_buffer.c/h`: Command buffer utilities
- `vulkan_fence.c/h`: Synchronization primitives
- `vulkan_pipeline.c/h`: Graphics pipeline setup
- `vulkan_buffer.c/h`: Buffer creation and management
- `vulkan_image.c/h`: Image/texture handling

**Platform Integration**:
- Windows: VK_KHR_win32_surface
- Linux: VK_KHR_xcb_surface (from XCB connection)

### Design Benefits

1. **Backend Agnostic**: Game code uses frontend API only
2. **Hot-Swappable**: Backend can be selected at runtime
3. **Platform Integration**: Backends use platform state for surface creation
4. **Extensibility**: Easy to add new backends (OpenGL, DirectX, Metal)
5. **Testing**: Mock backend for headless testing
6. **Future-Proof**: Render packet designed to expand with features

---

## Data Structures

### Dynamic Array (darray)

**File**: `engine/src/containers/darray.c/h`

**Purpose**: Type-safe, generic dynamic array (vector/ArrayList)

**Design**: Macro-based compile-time code generation with metadata header

### Memory Layout

```
[capacity: u64][length: u64][stride: u64][element_0][element_1]...[element_N-1]
^                                         ^
|                                         |
Metadata (internal)                       User pointer (returned)
```

**Metadata Fields**:
- `capacity`: u64 - Maximum elements before reallocation
- `length`: u64 - Current number of elements
- `stride`: u64 - Size of each element (sizeof(type))

### Core Macros

**Creation**:
```c
// Create with default capacity (1 element)
#define darray_create(type) \
    _darray_create(1, sizeof(type))

// Create with specific capacity
#define darray_reserve(type, capacity) \
    _darray_create(capacity, sizeof(type))
```

**Append/Remove**:
```c
// Push element to end (grows if needed)
#define darray_push(array, value) \
    {                                                     \
        typeof(value) temp = value;                       \
        array = _darray_push(array, &temp);               \
    }

// Pop last element into dest
#define darray_pop(array, dest_ptr) \
    _darray_pop(array, dest_ptr)

// Insert at specific index
#define darray_insert_at(array, index, value) \
    {                                                     \
        typeof(value) temp = value;                       \
        array = _darray_insert_at(array, index, &temp);   \
    }

// Remove at specific index
#define darray_pop_at(array, index, dest_ptr) \
    _darray_pop_at(array, index, dest_ptr)
```

**Queries**:
```c
#define darray_capacity(array) \
    _darray_field_get(array, DARRAY_CAPACITY)

#define darray_length(array) \
    _darray_field_get(array, DARRAY_LENGTH)

#define darray_stride(array) \
    _darray_field_get(array, DARRAY_STRIDE)

#define darray_length_set(array, value) \
    _darray_field_set(array, DARRAY_LENGTH, value)
```

**Cleanup**:
```c
#define darray_clear(array) \
    _darray_field_set(array, DARRAY_LENGTH, 0)

#define darray_destroy(array) \
    _darray_destroy(array)
```

### Internal Implementation

**Field Enum**:
```c
typedef enum {
    DARRAY_CAPACITY,
    DARRAY_LENGTH,
    DARRAY_STRIDE,
    DARRAY_FIELD_LENGTH
} darray_field;
```

**Field Access**:
```c
// Get metadata field value
static u64 _darray_field_get(void* array, darray_field field) {
    u64* header = (u64*)array - DARRAY_FIELD_LENGTH;
    return header[field];
}

// Set metadata field value
static void _darray_field_set(void* array, darray_field field, u64 value) {
    u64* header = (u64*)array - DARRAY_FIELD_LENGTH;
    header[field] = value;
}
```

**Growth Strategy**:
```c
// Called when push exceeds capacity
void* _darray_resize(void* array) {
    u64 capacity = darray_capacity(array);
    u64 stride = darray_stride(array);

    // Double capacity (2x growth)
    u64 new_capacity = capacity * DARRAY_RESIZE_FACTOR;  // FACTOR = 2

    // Reallocate: metadata + element array
    u64 header_size = DARRAY_FIELD_LENGTH * sizeof(u64);
    u64 new_size = header_size + (new_capacity * stride);

    void* temp = kallocate(new_size, MEMORY_TAG_DARRAY);
    kcopy_memory(temp, header, header_size + (length * stride));

    kfree(header, old_size, MEMORY_TAG_DARRAY);

    // Update capacity in new header
    _darray_field_set(temp, DARRAY_CAPACITY, new_capacity);

    return temp;
}
```

### Usage Example

```c
// Create array of integers
i32* numbers = darray_create(i32);

// Add elements
darray_push(numbers, 10);
darray_push(numbers, 20);
darray_push(numbers, 30);

// Query
u64 count = darray_length(numbers);  // 3
u64 cap = darray_capacity(numbers);  // 4 (after growth)

// Access elements
for (u64 i = 0; i < count; i++) {
    KINFO("numbers[%llu] = %d", i, numbers[i]);
}

// Insert at index
darray_insert_at(numbers, 1, 15);  // [10, 15, 20, 30]

// Remove element
i32 popped;
darray_pop_at(numbers, 2, &popped);  // popped = 20, array = [10, 15, 30]

// Cleanup
darray_destroy(numbers);
```

**Event System Usage**:
```c
// Event listeners array
registered_event* listeners = darray_create(registered_event);

// Add listener
registered_event event = {.listener = instance, .callback = on_event};
darray_push(listeners, event);

// Iterate listeners
for (u64 i = 0; i < darray_length(listeners); i++) {
    listeners[i].callback(code, sender, listeners[i].listener, context);
}
```

### Type Safety

**Using `typeof()` (GNU C extension)**:
```c
#define darray_push(array, value) \
    {                                                     \
        typeof(value) temp = value;  // Type-safe copy    \
        array = _darray_push(array, &temp);               \
    }
```

**Benefit**: Compiler enforces type matching at call site
```c
i32* ints = darray_create(i32);
darray_push(ints, 42);       // OK
darray_push(ints, "hello");  // Compilation error (type mismatch)
```

### Design Benefits

1. **Type Safety**: Compile-time type checking via `typeof()`
2. **Zero Overhead**: Macros expand to direct pointer arithmetic
3. **Automatic Growth**: 2x resize factor amortizes to O(1) push
4. **Memory Tracking**: All allocations tagged with MEMORY_TAG_DARRAY
5. **Cache-Friendly**: Contiguous memory layout
6. **Familiar API**: Similar to C++ std::vector
7. **Lightweight**: No vtables or function pointers

---

## Design Decisions

### 1. Pure C (No C++)

**Rationale**:
- Minimal runtime dependencies (no STL)
- Predictable memory layout and performance
- Maximum portability (easier to integrate with other languages)
- Explicit resource management (no RAII magic)
- Smaller binary size
- Easier to debug (no template error messages)

**Trade-offs**:
- Manual memory management (no destructors)
- Macro-based generics instead of templates
- Explicit initialization/shutdown instead of constructors/destructors

### 2. Layered Architecture with Strict Dependencies

**Rationale**:
- Clear module boundaries prevent tangled dependencies
- Testability: Each layer can be tested independently
- Replaceability: Swap implementations without touching higher layers
- Platform independence: Only platform layer touches OS

**Enforcement**:
- Header include hierarchy enforces dependency direction
- Platform layer has no dependencies on core systems
- Core systems don't know about renderer or game

### 3. Opaque Pointers for Platform State

**Rationale**:
- True platform abstraction without `#ifdef` above platform layer
- OS-specific types (HWND, xcb_connection_t) hidden from engine
- Enables cleaner testing (mock platform implementations)

**Implementation**:
```c
// Public: platform.h
typedef struct platform_state {
    void* internal_state;
} platform_state;

// Private: platform_win32.c
typedef struct internal_state {
    HINSTANCE h_instance;
    HWND hwnd;
} internal_state;
```

### 4. Event System Instead of Direct Calls

**Rationale**:
- Decouples subsystems (input doesn't know about application)
- Extensibility: Multiple listeners can subscribe to same event
- Hot-reload friendly: Listeners can register/unregister at runtime
- Propagation control: First handler can consume event

**Trade-offs**:
- Slight indirection overhead (function pointer call)
- Debugging harder (callback chain not visible in stack trace)

### 5. Double-Buffered Input State

**Rationale**:
- Enables "state change" queries: pressed, released, held
- Avoids race conditions (input updated once per frame)
- Game always sees consistent snapshot of input

**Implementation**:
```c
// Frame N
input_process_key(KEY_W, TRUE);  // Updates current
// Game queries: input_is_key_down(KEY_W) → TRUE
//               input_was_key_down(KEY_W) → FALSE (just pressed!)

// End of frame N
input_update();  // Copy current → previous

// Frame N+1
// Game queries: input_is_key_down(KEY_W) → TRUE
//               input_was_key_down(KEY_W) → TRUE (held down)
```

### 6. Tagged Memory Allocation

**Rationale**:
- Memory leak detection per subsystem
- Performance profiling (which systems allocate most?)
- Budget enforcement (e.g., textures < 512 MB)
- Debugging (categorize allocations in memory dumps)

**Future Extensions**:
- Per-tag memory pools
- Custom allocators per tag
- Memory usage visualization

### 7. Static Module State

**Rationale**:
- Simple C singleton pattern
- No need to pass state pointers everywhere
- Clear initialization/shutdown lifecycle

**Pattern**:
```c
// input.c (internal)
typedef struct input_state {
    keyboard_state keyboard_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
} input_state;

static input_state* state_ptr;  // Internal state

// Public API
void input_initialize() {
    state_ptr = kallocate(sizeof(input_state), MEMORY_TAG_INPUT);
    kzero_memory(state_ptr, sizeof(input_state));
}

void input_shutdown() {
    kfree(state_ptr, sizeof(input_state), MEMORY_TAG_INPUT);
}
```

**Trade-offs**:
- Not thread-safe (acceptable for single-threaded game loop)
- Global mutable state (but isolated per module)

### 8. Macro-Based Generic Containers

**Rationale**:
- Type safety at compile time (via `typeof()`)
- Zero runtime overhead (macros expand to direct code)
- No code bloat (macros don't generate code per instantiation like C++ templates)
- Familiar syntax for C developers

**Alternative Rejected**: `void*` arrays
- Would lose type safety
- Require casting at every access
- Error-prone (wrong casts compile but crash at runtime)

### 9. Function Pointer Vtables for Polymorphism

**Used in**: renderer_backend, game interface

**Rationale**:
- Pure C polymorphism (no virtual methods)
- Runtime polymorphism (select backend at runtime)
- DLL-friendly (function pointers work across DLL boundaries)
- Testability (mock implementations easy to inject)

**Example**:
```c
// Backend interface
typedef struct renderer_backend {
    b8 (*initialize)(...);
    void (*shutdown)(...);
} renderer_backend;

// Vulkan implementation
static b8 vulkan_initialize(...) { ... }
static void vulkan_shutdown(...) { ... }

renderer_backend vulkan_backend = {
    .initialize = vulkan_initialize,
    .shutdown = vulkan_shutdown
};

// Usage
backend->initialize(backend, ...);  // Calls vulkan_initialize
```

### 10. Single-Threaded Game Loop

**Rationale**:
- Simplicity: No synchronization, no race conditions
- Predictability: Deterministic execution order
- Debugging: Easier to trace execution
- Sufficient for most games (60 FPS target achievable)

**Future Extensions**:
- Job system for parallel tasks (physics, rendering)
- Multi-threaded renderer (command buffer generation)

### 11. Frame-Paced Update with Target FPS

**Implementation**:
```c
f64 target_frame_seconds = 1.0 / 60.0;  // 60 FPS
clock_update(&clock);
f64 frame_start = clock.elapsed;

game_update(delta);
game_render(delta);

f64 frame_elapsed = platform_get_absolute_time() - frame_start;
if (frame_elapsed < target_frame_seconds) {
    platform_sleep((target_frame_seconds - frame_elapsed) * 1000);
}
```

**Rationale**:
- Consistent frame rate (not tied to V-Sync)
- Energy efficient (sleep when ahead of schedule)
- Predictable physics (fixed delta time option possible)

### 12. Minimal External Dependencies

**Current Dependencies**:
- **Core Engine**: C standard library only (stdio, stdlib, string, time)
- **Windows**: Win32 API (windows.h, windowsx.h)
- **Linux**: X11/XCB (libx11-dev, libxkbcommon-x11-dev)
- **Renderer**: Vulkan SDK (planned)

**Rationale**:
- Easier builds (fewer dependencies to install)
- Smaller binaries (no large libraries linked)
- Portability (fewer platform-specific quirks)
- Security (smaller attack surface)

### 13. Early Initialization Guarantees

**Initialization Order** (in `entry.h` main):
```c
initialize_memory();       // 1. Memory first (everyone needs allocations)
create_game(&game_inst);  // 2. Create game instance
application_create(...);   // 3. Initialize application
    initialize_logging();  //    3a. Logging (for diagnostics)
    input_initialize();    //    3b. Input (before platform events)
    event_initialize();    //    3c. Events (before firing events)
    platform_startup();    //    3d. Platform (window creation)
    game_initialize();     //    3e. Game (after all systems ready)
application_run();         // 4. Run game loop
```

**Rationale**:
- Prevents uninitialized subsystem usage
- Each subsystem can safely use lower layers
- Clear diagnostic messages if initialization fails (via KFATAL)

### 14. Compiler Feature Detection

**File**: `defines.h`

```c
// Static assertions
#ifdef __clang__
    #define STATIC_ASSERT _Static_assert
#else
    #define STATIC_ASSERT static_assert
#endif

// Type size verification
STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes");
```

**Rationale**:
- Catch platform-specific type size issues at compile time
- Compiler compatibility (Clang, GCC, MSVC)
- Fail fast if assumptions violated

---

## Summary

### Architecture Strengths

1. **Modularity**: Each subsystem is independently testable and replaceable
2. **Portability**: Platform abstraction enables Windows/Linux/future Mac support
3. **Extensibility**: Renderer backends, game types, and systems easily added
4. **Memory Safety**: Tagged allocation and leak detection ready
5. **Performance**: Minimal abstraction overhead, event-driven architecture
6. **Maintainability**: Clear responsibilities, consistent interfaces
7. **Scalability**: Designed for 3D rendering, physics, UI, and editor integration
8. **Pure C**: No STL dependencies, portable, minimal runtime overhead

### Current Implementation Status

- ✅ **Complete**: Platform layer (Windows/Linux), input, events, logging, memory, clock, containers
- ✅ **Functional**: Application lifecycle, game interface, testbed sample
- 🚧 **Scaffolding**: Renderer (interfaces defined, Vulkan implementation pending)
- 📋 **Planned**: Physics, UI, asset loading, multithreading, editor

### Future Expansion Points

1. **Renderer**: Vulkan backend implementation (primary target)
2. **Asset System**: Texture, mesh, shader loading with memory management
3. **Physics**: Collision detection, rigid body dynamics
4. **UI**: Immediate mode GUI for debug/editor tools
5. **Job System**: Multi-threaded task scheduling
6. **Editor**: Level editor, asset browser, profiler
7. **Networking**: Multiplayer support (optional)
8. **Scripting**: Lua or custom scripting integration (optional)

This architecture represents a **solid foundation for a modern, cross-platform 3D engine** with exceptional separation of concerns and forward-thinking extensibility design.

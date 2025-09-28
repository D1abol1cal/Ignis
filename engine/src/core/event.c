#include "core/event.h"
#include "core/kmemory.h"

typedef struct registered_event {
    void* listener;
    PFN_on_event callback;
} registered_event;

typedef struct event_code_entry {
    registered_event* events;
} event_code_entry;

//should be more than enough codes
#define MAX_MESSAGE_CODES 16384

//state structure
typedef struct event_system_state {
    //lookup tale for event codes
    event_code_entry registered[MAX_MESSAGE_CODES];
} event_system_state;

//Event system internal state
static b8 is_initialized = FALSE;
static event_system_state state;



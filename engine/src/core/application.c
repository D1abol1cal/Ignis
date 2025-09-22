#include "application.h"
#include "logger.h"
#include "platform/platform.h"

typedef struct application_state {
    b8 is_running;
    b8 is_suspended;
    platform_state platform;
    i16 width;
    i16 height;
    f64 last_time;
} application_state;

static b8 initialized = FALSE;
static application_state app_state;

b8 application_create(application_config* config) {
    if(initialized) {
        KERROR("Application already initialized!");
        return FALSE;
    }

    //Initialize subsystems.

    //initialize logging
    initialize_logging();

    // TODO: //Remove this
    KFATAL("A test message: %f", 3.14f);
    KERROR("A test message: %f", 3.14f);
    KDEBUG("A test message: %f", 3.14f);
    KTRACE("A test message: %f", 3.14f);

    app_state.is_running = TRUE;
    app_state.is_suspended = FALSE;

    if(!platform_startup(&app_state.platform, 
        config->name, 
        config->start_pox_x, 
        config->start_pos_y, 
        config->start_width, 
        config->start_height)) {
            KERROR("Failed to start platform!");
            return FALSE;
    }

    initialized = TRUE;

    return TRUE;
}

b8 application_run() {
    while(app_state.is_running) {
        if(!platform_pump_messages(&app_state.platform)) {
            app_state.is_running = FALSE;
            break;
        }
    }

    app_state.is_running = FALSE;

    platform_shutdown(&app_state.platform);

    return TRUE;
}
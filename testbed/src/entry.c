#include <entry.h>
#include "game.h"

// TODO: Remove this
#include <core/kmemory.h>
#include <core/logger.h>


//Definition of the function to create a game.
b8 create_game(game* out_game) {

    //application configuration
    out_game->app_config.start_pox_x = 100;
    out_game->app_config.start_pos_y = 100;
    out_game->app_config.start_width = 1280;
    out_game->app_config.start_height = 720;
    out_game->app_config.name = "Ignis Engine Testbed";

    out_game->update = game_update;
    out_game->render = game_render;
    out_game->initialize = game_initialize;
    out_game->on_resize = game_on_resize;

    //create the game state.
    out_game->state = kallocate(sizeof(game_state), MEMORY_TAG_GAME);

    return TRUE;
}


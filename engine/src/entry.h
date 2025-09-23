#pragma once

#include "core/application.h"
#include "core/logger.h"
#include "game_types.h"

//Externall-defined function to create a game.
extern b8 create_game(game* out_game);

//The main entry point for the application.

int main(void) {

    //Request the game instance from the application.
    game game_inst;
    if(!create_game(&game_inst)) {
        KERROR("Failed to create game instance!");
        return -1;
    }

    //Ensure the function pointers exist.
    if(!game_inst.initialize || !game_inst.update || !game_inst.render || !game_inst.on_resize) {
        KERROR("Game instance is missing required function pointers!");
        return -2;
    }


    //Initialization
    if(!application_create(&game_inst)) {
        KINFO("Application failed to create!");
        return 1;
    }

    //Begin the game loop
    if(!application_run()) {
        KINFO("Application did not shutdown gracefully!");
        return 2;
    }

    return 0;
}


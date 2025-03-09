#include "simple_logger.h"
#include "gf2d_draw.h"
#include "player.h"
#include "world.h"
#include "level.h"
#include "ui.h"

typedef struct WorldManager_S {
	Entity*			player;
	Uint8			close_game;
	WorldState		state;
	float			pause_time;
}WorldManager;

static WorldManager world_manager = { 0 };

void world_gamestart();
void world_close();

void world_init() {
	world_manager.close_game = 0;
	world_manager.state = WORLD_MAINMENU;

	// initialize level_manager
	level_manager_init("config/level_list.cfg");
	
	atexit(world_close);
}

void world_gamestart() {
	Level* level;
	SJson* data;

	// load first level
	level_load(0);

	level = get_curr_level();
	if (!level) {
		slog("failed to load first level");
		world_manager.close_game = 1;
		return;
	}

	// load player
	data = sj_load("def/player_data_init.def");
	if (!data) {
		slog("def file not found");
		world_manager.close_game = 1;
		return;
	}

	world_manager.player = player_spawn(level->player_spawn, data);
	sj_free(data);
	if (!world_manager.player) {
		world_manager.close_game = 1;
		return;
	}
}

void world_close() {
	memset(&world_manager, 0, sizeof(WorldManager));
}

void world_update() {
	//const Uint8* keys;
	//keys = SDL_GetKeyboardState(NULL);

	switch (world_manager.state) {
		case WORLD_GAMESTART:
			world_gamestart();
			world_manager.state = WORLD_INGAME;

			break;

		case WORLD_INGAME:
			level_update();

			entity_apply_grav_all();
			entity_think_all();
			entity_update_all();
			entity_draw_all();

			drawUI(world_manager.state);

			if (gfc_input_command_released("pause"))
				world_manager.state = WORLD_PAUSEMENU;

			break;

		case WORLD_PAUSEMENU:
			drawUI(world_manager.state);

			break;		

		case WORLD_PLAYERDEAD:

			break;

		case WORLD_LEVELCOMPLETE:
			// load next level here


			break;

		case WORLD_GAMECLOSE:
			free_all_entities();
			level_curr_close();
			
			world_manager.state = WORLD_MAINMENU;

			break;

		case WORLD_CLOSE:
			world_manager.close_game = 1;

			break;

		default: //WORLD_MAINMENU
			drawUI(world_manager.state); // draw menu and check input	
	}

	//if (keys[SDL_SCANCODE_ESCAPE]) world_manager.close_game = 1; // exit condition
	//gf2d_draw_rect(RES, GFC_COLOR_RED);
}

/*
WorldState getWorldState() {
	return world_manager.state;
}
*/

void change_world_state(WorldState new_state) {
	world_manager.state = new_state;
}

Uint8 close_game_check() {
	return world_manager.close_game;
}

#include "simple_logger.h"
#include "gf2d_draw.h"
#include "world.h"
//#include "camera.h"
#include "player.h"
#include "level.h"
#include "ui.h"

typedef struct WorldManager_S {
	Entity*			player;
	GFC_List*		enemy_list;
	//GFC_List*		item_list;
	Uint8			close_game;
	WorldState		state;
}WorldManager;

static WorldManager world_manager = { 0 };

void world_gamestart();
void world_close();

void world_init() {
	world_manager.close_game = 0;
	world_manager.state = WORLD_MAINMENU;

	// initialize level_manager
	world_manager.enemy_list = gfc_list_new();
	//world_manager.item_list = gfc_list_new();
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
	gfc_list_delete(world_manager.enemy_list);
	//gfc_list_delete(world_manager.item_list);
	memset(&world_manager, 0, sizeof(WorldManager));
}

void world_update() {
	switch (world_manager.state) {
		case WORLD_GAMESTART:
			world_gamestart();
			//camera_init();
			world_manager.state = WORLD_INGAME;

			break;

		case WORLD_INGAME:
			level_update();

			entity_apply_grav_all();
			entity_think_all();
			entity_update_all();
			entity_draw_all();

			//camera_update(world_manager.player, get_curr_level()->level_size);
			drawUI(world_manager.state);

			if (gfc_input_command_released("pause"))
				world_manager.state = WORLD_PAUSEMENU;

			break;	

		case WORLD_RESTART_LEVEL:
			free_all_level_entities();
			level_curr_close();
			gfc_list_clear(world_manager.enemy_list);
			restart_level(world_manager.player);

			world_manager.state = WORLD_INGAME;
			break;

		case WORLD_LOAD_NEXT_LEVEL:
			free_all_level_entities();
			level_curr_close();
			gfc_list_clear(world_manager.enemy_list);
			load_next_level(world_manager.player);

			world_manager.state = WORLD_INGAME;

			break;

		case WORLD_GAMECLOSE:
			free_all_entities();
			level_curr_close();
			gfc_list_clear(world_manager.enemy_list);
			
			world_manager.state = WORLD_MAINMENU;

			break;

		case WORLD_CLOSE:
			world_manager.close_game = 1;

			break;

		default: //WORLD_MAINMENU, WORLD_PAUSEMENU, WORLD_PLAYERDEAD, WORLD_LEVELCOMPLETE, WORLD_GAME_COMPLETE
			drawUI(world_manager.state); // draw menu and check input	
	}
}

Uint8 checkFramePass(float then) {
	return CURRENT_TIME - then > FRAME_DUR;
}

void change_world_state(WorldState new_state) {
	world_manager.state = new_state;
}

Uint8 close_game_check() {
	return world_manager.close_game;
}

void* get_player() {
	return world_manager.player;
}

void* get_player_data() {
	return world_manager.player->data;
}

GFC_List* get_enemy_list() {
	return world_manager.enemy_list;
}

GFC_Vector2D get_player_pos() {
	return world_manager.player->position;
}

/*
GFC_List* get_item_list() {
	return world_manager.item_list;
}
*/
#include "simple_logger.h"
#include "gfc_list.h"
#include "player.h"
#include "world.h"
#include "level.h"

typedef struct WorldManager_S {
	Entity*			player;
	Uint8			_done;
	// GameState

	// insert UI data
	// insert level data
}WorldManager;

static WorldManager world_manager = { 0 };

void world_close();

void world_done_change() {
	world_manager._done = world_manager._done ? 0 : 1;
}

Uint8 world_done_check() {
	return world_manager._done;
}

void world_init() {
	Level* level;

	world_manager._done = 0;

	level_manager_init();
	level = get_curr_level();
	world_manager.player = player_spawn(level->player_spawn);
	if (!world_manager.player) {
		slog("failed to initialize player");
		world_manager._done = 1;
		return;
	}

	atexit(world_close);
}

void world_close() {
	entity_system_close();

}

void world_update() {
	level_update();

	entity_apply_grav_all();
	entity_think_all();
	entity_update_all();
	entity_draw_all();
}

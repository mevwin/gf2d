#include "simple_logger.h"
#include "gfc_list.h"
#include "player.h"
#include "world.h"
#include "level.h"

typedef struct WorldManager_S {
	Entity*			player;
	GFC_List*		enemy_list;
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

	world_manager.enemy_list = gfc_list_new();
	level_manager_init();
	level = get_curr_level();
	world_manager.player = player_spawn(level->player_spawn);
	if (!world_manager.player) {
		world_manager._done = 1;
		return;
	}

	atexit(world_close);
}

void world_close() {
	entity_system_close();

	gfc_list_clear(world_manager.enemy_list);
	gfc_list_delete(world_manager.enemy_list);

	memset(&world_manager, 0, sizeof(WorldManager));
}

void world_update() {
	level_update();

	entity_apply_grav_all();
	entity_think_all();
	entity_update_all();
	entity_draw_all();
}

void world_append_enemy(void* enemy) {
	Entity* enem;

	enem = (Entity*)enemy;
	if (!enem) {
		slog("no enemy given");
		return;
	}

	gfc_list_append(world_manager.enemy_list, enem);
}

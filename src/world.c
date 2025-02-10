#include "simple_logger.h"
#include "gf2d_draw.h"
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
	GFC_List*		def_strings;
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
	SJson* def_strings, *string_list, *data;
	int i;

	world_manager._done = 0;

	// init lists
	world_manager.enemy_list = gfc_list_new();

	world_manager.def_strings = gfc_list_new();
	def_strings = sj_load("config/def_strings.cfg");
	string_list = sj_object_get_value(def_strings, "list");
	for (i = 0; i < string_list->v.array->count; i++) {
		gfc_list_append(
			world_manager.def_strings,
			sj_object_get_string(sj_array_get_nth(string_list, i), "path")
		);
	}

	level_manager_init();
	level = get_curr_level();

	// load player
	data = sj_load(gfc_list_nth(world_manager.def_strings, 0));
	if (!data){
		slog("def file not found");
		world_manager._done = 1;
		return;
	}

	world_manager.player = player_spawn(level->player_spawn, data);
	sj_free(data);
	if (!world_manager.player) {
		sj_free(def_strings);
		world_manager._done = 1;
		return;
	}

	sj_free(def_strings);
	atexit(world_close);
}

void world_close() {
	entity_system_close();

	gfc_list_clear(world_manager.def_strings);
	gfc_list_delete(world_manager.def_strings);

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
	//gf2d_draw_rect(RES, GFC_COLOR_RED);
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

#include "simple_logger.h"
#include "gfc_list.h"
#include "player.h"
#include "world.h"
#include "level.h"

typedef struct WorldManager_S {
	Entity*			player;

	// GameState

	// insert UI data
	// insert level data
}WorldManager;

static WorldManager world_manager = { 0 };

void world_close();

void world_init() {
	world_manager.player = player_spawn(gfc_vector2d(400, 349));



	atexit(world_close);
}

void world_close() {
	entity_system_close();

}

void world_update() {
	level_update();

	entity_think_all();
	entity_update_all();
	entity_draw_all();
}


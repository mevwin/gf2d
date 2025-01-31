#include "simple_logger.h"
#include "gf2d_draw.h"
#include "world.h"
#include "level.h"
#include "entity.h"

typedef struct LevelManager_S {
	Level*			curr_level;
	GFC_List*		level_list;
	Uint8			level_num;
}LevelManager;

static LevelManager level_manager = { 0 };
static Level* level;

void level_manager_close();

void level_manager_init() {
	//level_manager.level_list = gfc_list_new();

	// load first level
	level_manager.curr_level = level_load(0);

	atexit(level_manager_close);
}

void level_manager_close() {

	free(level_manager.curr_level);
	//gfc_list_delete(level_manager.level_list);
}

Level* level_load(Uint8 index) {
	Level* level;

	level = gfc_allocate_array(sizeof(Level), 1);

	//level->room_num = 0;
	level->ground = gfc_rect(0, 600, 1200, 120);
	level->player_spawn = gfc_vector2d(600, 200);

	return level;
}

void level_update() {
	// draw ground
	gf2d_draw_rect_filled(level_manager.curr_level->ground, GFC_COLOR_BLACK);
}

void level_close(Level* level) {

}

Level* get_curr_level() {
	return level_manager.curr_level;
}

Uint8 ground_collision(void* ent) {
	Entity* self;
	GFC_Edge2D edge;
	GFC_Vector2D p1, p2;

	self = (Entity*) ent;

	p1 = gfc_vector2d(0, level_manager.curr_level->ground.y + 1.0f);
	p2 = gfc_vector2d(1200, level_manager.curr_level->ground.y + 1.0f);

	edge = gfc_edge_from_vectors(p1, p2);

	if (gfc_edge_rect_intersection(
			edge,
			self->hurtbox.s.r))
	{
		return 1;
	}
	else
		return 0;
}
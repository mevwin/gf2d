#include "entity.h"
#include "gf2d_draw.h"
#include "level.h"
#include "simple_logger.h"
#include "world.h"
#include "player.h"

typedef struct LevelManager_S {
	Level*			curr_level;
	GFC_List*		level_list;
	Uint8			level_num;
}LevelManager;

static LevelManager level_manager = { 0 };

void level_manager_close();
Ground* create_ground(GFC_Rect dimen, GFC_Color color);
void level_find_nearest_ground();

void level_manager_init() {
	level_manager.level_list = gfc_list_new();

	// load first level
	level_manager.curr_level = level_load(0);

	atexit(level_manager_close);
}

void level_manager_close() {
	Level* level;
	int i;

	for (i = 0; i < level_manager.level_list->count; i++) {
		level = (Level*) gfc_list_nth(level_manager.level_list, i);
		level_close(level);
	}
	gfc_list_clear(level_manager.level_list);
	gfc_list_delete(level_manager.level_list);
	memset(&level_manager, 0, sizeof(LevelManager));
}

Ground* create_ground(GFC_Rect dimen, GFC_Color color) {
	Ground* ground;

	ground = gfc_allocate_array(sizeof(Ground), 1);
	if (!ground)
		return NULL;

	ground->dimensions = dimen;
	ground->region = gfc_vector2d(ground->dimensions.x,
		ground->dimensions.x + ground->dimensions.w);
	ground->color = color;

	return ground;
}

Level* level_load(Uint8 index) {
	Level* level;
	Ground* ground1, *ground2;

	level = gfc_allocate_array(sizeof(Level), 1);

	level->ground_list = gfc_list_new();
	
	ground1 = create_ground(gfc_rect(400, 600, 800, 120), GFC_COLOR_BLACK);
	gfc_list_append(level->ground_list, ground1);

	ground2 = create_ground(gfc_rect(0, 400, 400, 320), GFC_COLOR_YELLOW);
	gfc_list_append(level->ground_list, ground2);

	level->curr_ground = gfc_list_nth(level->ground_list, 0);

	level->player_spawn = gfc_vector2d(600, 200);

	return level;
}

void level_close(Level* level) {
	gfc_list_foreach(level->ground_list, free);
	gfc_list_clear(level->ground_list);
	gfc_list_delete(level->ground_list);

	free(level);
}

void level_update() {
	int i;
	Ground* ground;

	// draw ground
	for (i = 0; i < level_manager.curr_level->ground_list->count; i++) {
		ground = (Ground*)gfc_list_nth(level_manager.curr_level->ground_list, i);
		gf2d_draw_rect_filled(ground->dimensions, ground->color);
	}
	//gf2d_draw_rect_filled(level_manager.curr_level->ground, GFC_COLOR_BLACK);
}

Level* get_curr_level() {
	return level_manager.curr_level;
}

float get_ground_level() {
	return level_manager.curr_level->curr_ground->dimensions.y - 1.0f;
}

void level_find_nearest_ground() {
	Ground* ground;
	GFC_Vector2D* player_pos;
	int i;

	player_pos = get_player_pos();

	for (i = 0; i < level_manager.curr_level->ground_list->count; i++) {
		ground = (Ground*) gfc_list_nth(level_manager.curr_level->ground_list, i);
		
		if (player_pos->x >= ground->region.x && player_pos->x < ground->region.y) {
			level_manager.curr_level->curr_ground = ground;
			slog("%d", i);
		}
	}
}

Uint8 ground_collision(void* ent) {
	Entity* self;
	GFC_Edge2D edge, bottom;
	GFC_Vector2D p1, p2;

	self = (Entity*) ent;
	if (!self) {
		slog("no entity");
		return 0;
	}

	bottom = get_bottom_edge(self->boundbox.s.r); // player's bottom edge
	level_find_nearest_ground();

	p1 = gfc_vector2d(0, level_manager.curr_level->curr_ground->dimensions.y - 1.0f);
	p2 = gfc_vector2d(1200, level_manager.curr_level->curr_ground->dimensions.y - 1.0f);

	edge = gfc_edge_from_vectors(p1, p2);
	
	if (roundf(bottom.y1) >= edge.y1) {
		return 1;
	}
	else return 0;

	//return gfc_edge_intersect(bottom, edge);
}
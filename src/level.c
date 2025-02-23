#include "simple_logger.h"
#include "gf2d_draw.h"
#include "world.h"
#include "collisions.h"
#include "player.h"
#include "enemy.h"

typedef struct LevelManager_S {
	Level*			curr_level;
	GFC_List*		level_list; // a list of filepaths for levels
	Uint8			level_num;
	Uint8			w_collision;  // 0 for left, 1 for right
}LevelManager;

//NOTE: ONLY ONE LEVEL LOADED AT A TIME

static LevelManager level_manager = { 0 };

void level_manager_close();
Ground* create_ground(GFC_Rect dimen, GFC_Color color);
Wall* create_wall(GFC_Edge2D dimen, Uint8 type);

void level_manager_init(const char* filename){	
	SJson* level_def, *level_list;
	int i;
	
	// load level list
	level_manager.level_list = gfc_list_new();
	level_def = sj_load(filename);
	level_list = sj_object_get_value(level_def, "list");

	for (i = 0; i < level_list->v.array->count; i++) {
		gfc_list_append(
			level_manager.level_list,
			sj_object_get_string(sj_array_get_nth(level_list, i), "path")
			);
	}
	level_manager.level_num = 0;

	// load first level
	level_manager.curr_level = level_load(level_manager.level_num);
	if (!level_manager.curr_level) {
		slog("failed to initialiaze first level");
		world_done_change();
		return;
	}

	sj_free(level_def);

	atexit(level_manager_close);
}

void level_manager_close() {
	int i;

	level_close(level_manager.curr_level);

	for (i = 0; i < level_manager.level_list->count; i++) {
		//free(gfc_list_nth(level_manager.level_list, i));
	}
	gfc_list_clear(level_manager.level_list);
	gfc_list_delete(level_manager.level_list);
	memset(&level_manager, 0, sizeof(LevelManager));
}

Level* level_load(Uint8 index) {
	Level* level;
	Ground* ground;
	Wall* wall;
	GFC_Rect dimen;
	GFC_Vector4D rec_buf;
	SJson *level_obj, *level_data, *ground_list, *ground_data;
	Uint8 load_walls, load_ceil;
	int i;

	level = gfc_allocate_array(sizeof(Level), 1);
	if (!level) {
		slog("failed to initialize level");
		return NULL;
	}
	level_obj = sj_load(gfc_list_nth(level_manager.level_list, index));
	level_data = sj_object_get_value(level_obj, "level");
	if (!level_data) {
		slog("level not found");
		world_done_change();
		return NULL;
	}

	level->ground_list = gfc_list_new();
	level->wall_list = gfc_list_new();

	// player spawn
	sj_object_get_vector2d(level_data, "player_spawn", &level->player_spawn);

	// ground list
	ground_list = sj_object_get_value(level_data, "ground_list");
	for (i = 0; i < ground_list->v.array->count; i++) {
		ground_data = sj_array_get_nth(ground_list, i);
		
		sj_object_get_vector4d(ground_data, "dimensions", &rec_buf);
		dimen = gfc_rect_from_vector4(rec_buf);
		
		sj_object_get_uint8(ground_data, "walls", &load_walls);
		sj_object_get_uint8(ground_data, "ceiling", &load_ceil);
		
		ground = create_ground(dimen, sj_object_get_color(ground_data, "color"));
		if (!ground) {
			slog("ground failed to be created");
			continue;
		}
		gfc_list_append(level->ground_list, ground);

		// initalize walls if toggled
		if (load_walls) {
			// create left wall
			wall = create_wall(get_edge_from_rect(ground->dimensions, 3), 0);
			gfc_list_append(level->wall_list, wall);

			wall = create_wall(get_edge_from_rect(ground->dimensions, 2), 1);
			gfc_list_append(level->wall_list, wall);
		}

		if (load_ceil) {
			ground->ceil_flag = 1;
			//slog("true");
		}
	}
	// create level bounds
	//gfc_list_append();

	// enemy spawns
	//enemy_spawn(BRUISER, gfc_vector2d(300, 200));

	sj_free(level_data);
	return level;
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

Wall* create_wall(GFC_Edge2D dimen, Uint8 type) {
	Wall* wall;

	wall = gfc_allocate_array(sizeof(Wall), 1);
	if (!wall) {
		slog("failed to allocate memory for wall");
		return NULL;
	}

	wall->dimensions = dimen;
	wall->type = type;

	return wall;
}

void level_close(Level* level) {
	if (!level) {
		slog("no level to close");
		return;
	}

	gfc_list_foreach(level->ground_list, free);
	gfc_list_clear(level->ground_list);
	gfc_list_delete(level->ground_list);

	gfc_list_foreach(level->wall_list, free);
	gfc_list_clear(level->wall_list);
	gfc_list_delete(level->wall_list);

	free(level);
}

void level_update() {
	int i;
	//float offset;
	Ground* ground;

	//offset = 1.0f;

	// draw ground
	for (i = 0; i < level_manager.curr_level->ground_list->count; i++) {
		ground = (Ground*)gfc_list_nth(level_manager.curr_level->ground_list, i);
		gf2d_draw_rect_filled(ground->dimensions, ground->color);
		
		/* Testing moving collision
		ground->dimensions.x += offset;
		gfc_vector2d_add(ground->region, ground->region, gfc_vector2d(offset, offset));
		if(ground->dimensions.x + ground->dimensions.w + offset >= ground->region.y || 
			ground->dimensions.x + offset <= ground->region.x)
			offset = -offset;
		*/
	}
	//gf2d_draw_rect_filled(level_manager.curr_level->ground, GFC_COLOR_BLACK);
}

Level* get_curr_level() {
	return level_manager.curr_level;
}

Uint8 entity_keep_in_bounds(void* e, Uint8 edge_type) {
	GFC_Edge2D screen_edge, e_edge;
	Entity* self;
	//GFC_Vector2D p1, p2;
	float offset;

	self = (Entity*) e;
	if (!self) return;
	if (self->bounds) self->bounds(self);
	// if no unique bounds function, at least restrict entity to viewspace
	offset = 1.0f;

	screen_edge = get_edge_from_rect(RES, edge_type + 2);
	e_edge = get_edge_from_rect(self->boundbox.s.r, edge_type + 2);
	e_edge.x1 += !edge_type ? self->velocity.x : -self->velocity.x;

	//slog("p: %f, s: %f", e_edge.x1, screen_edge.x1);

	if ((edge_type && e_edge.x1 <= screen_edge.x1 + offset) ||
		(!edge_type && e_edge.x1 >= screen_edge.x1 - offset))
		return 1;
	else
		return 0;

	/*
	screen_edge = get_edge_from_rect(RES, 1);
	e_edge = get_edge_from_rect(self->boundbox.s.r, 1);
	if (e_edge.y1 - self->velocity.y <= screen_edge.y1 + offset)
		return 0;

	screen_edge = get_edge_from_rect(RES, 0);
	e_edge = get_edge_from_rect(self->boundbox.s.r, 0);
	if (e_edge.y1 + self->velocity.y >= screen_edge.y1 - offset)
		return 0;
	*/
}

/**
* apply bit masking/bitwise operations for layering
* ex:
*	01000
*  &?????
*	0?000
* 
* means if the bits match, objects are the same layer
*/

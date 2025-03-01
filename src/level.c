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
Ground* create_ground(SJson* ground_data, Level* level);
Wall* create_wall(GFC_Edge2D dimen, Uint8 type);
Platform* create_platform(SJson* plat_data);
void update_platforms();
void move_platform(Platform* plat);

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
	Platform* plat;
	SJson *level_obj, *level_data, *list, *data;
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
	level->platform_list = gfc_list_new();

	// player spawn
	sj_object_get_vector2d(level_data, "player_spawn", &level->player_spawn);

	// ground list
	list = sj_object_get_value(level_data, "ground_list");
	if (!list) return;
	for (i = 0; i < list->v.array->count; i++) {
		data = sj_array_get_nth(list, i);
		if (!data) continue;
		
		ground = create_ground(data, level);
		if (!ground) {
			slog("ground failed to be created");
			continue;
		}
		
		gfc_list_append(level->ground_list, ground);
	}

	// platform list
	list = sj_object_get_value(level_data, "platform_list");
	if (!list) return;
	for (i = 0; i < list->v.array->count; i++) {
		data = sj_array_get_nth(list, i);
		if (!data) continue;

		plat = create_platform(data);
		gfc_list_append(level->platform_list, plat);
	}

	// create level bounds
	//gfc_list_append();

	// enemy spawns
	enemy_spawn(BRUISER, gfc_vector2d(300, 200));

	sj_free(level_data);
	return level;
}

Ground* create_ground(SJson* ground_data, Level* level) {
	Ground* ground;
	Wall* wall;
	GFC_Vector4D rec_buf;

	ground = gfc_allocate_array(sizeof(Ground), 1);
	if (!ground || !ground_data)
		return NULL;

	sj_object_get_vector4d(ground_data, "rect", &rec_buf);
	ground->dimensions = gfc_rect_from_vector4(rec_buf);
	ground->region = gfc_vector2d(ground->dimensions.x,
								ground->dimensions.x + ground->dimensions.w);

	ground->color = sj_object_get_color(ground_data, "color");

	// initalize walls if toggled
	sj_object_get_uint8(ground_data, "walls", &ground->wall_flag);
	if (ground->wall_flag) {
		// create left wall
		wall = create_wall(get_edge_from_rect(ground->dimensions, 3), 0);
		gfc_list_append(level->wall_list, wall);


		wall = create_wall(get_edge_from_rect(ground->dimensions, 2), 1);
		gfc_list_append(level->wall_list, wall);
	}

	sj_object_get_uint8(ground_data, "ceiling", &ground->ceil_flag);

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

Platform* create_platform(SJson* plat_data) {
	Platform* plat;
	GFC_Vector4D rec_buf;
	GFC_Rect dimen;
	Uint8 ibuf;

	plat = gfc_allocate_array(sizeof(Platform), 1);
	if (!plat || !plat_data) {
		slog("failed to allocate memory for platform");
		return NULL;
	}

	sj_object_get_vector4d(plat_data, "rect", &rec_buf);
	dimen = gfc_rect_from_vector4(rec_buf);
	plat->dimensions = dimen;
	plat->region = gfc_vector2d(plat->dimensions.x,
								plat->dimensions.x + plat->dimensions.w);

	sj_object_get_uint8(plat_data, "pass_through", &plat->pass_through);

	sj_object_get_uint8(plat_data, "starting_move", &ibuf);
	plat->moveType = (PlatformMove)ibuf;

	sj_object_get_uint8(plat_data, "moving", &plat->moving);
	if (plat->moving) {
		sj_object_get_vector2d(plat_data, "move_speed", &plat->move_speed);
		sj_object_get_vector4d(plat_data, "move_bounds", &plat->move_bounds);
	}

	
	return plat;
}

void update_platforms() {
	Platform* plat;
	int i;

	for (i = 0; i < level_manager.curr_level->platform_list->count; i++) {
		plat = (Platform*)gfc_list_nth(level_manager.curr_level->platform_list, i);
		if (!plat) continue;

		// draw plat
		gf2d_draw_rect_filled(plat->dimensions, GFC_COLOR_BLACK);

		// move platform if allowed
		if (plat->moving)
			move_platform(plat);
	}
}

void move_platform(Platform* plat) {
	if (!plat) return;
	
	switch (plat->moveType) {
		case PLATFORM_MOVE_LEFT:
			plat->dimensions.x -= plat->move_speed.x;
			if (plat->dimensions.x <= plat->move_bounds.x)
				plat->moveType = PLATFORM_MOVE_RIGHT;

			break;

		case PLATFORM_MOVE_RIGHT:
			plat->dimensions.x += plat->move_speed.x;
			if (plat->dimensions.x + plat->dimensions.w >= plat->move_bounds.y)
				plat->moveType = PLATFORM_MOVE_LEFT;

			break;

		default:
			// do nothing
			;
	}

	// update region
	plat->region = gfc_vector2d(plat->dimensions.x,
								plat->dimensions.x + plat->dimensions.w);
}

void level_close(Level* level) {
	if (!level) {
		slog("no level to close");
		return;
	}

	gfc_list_foreach(level->ground_list, free);
	gfc_list_delete(level->ground_list);

	gfc_list_foreach(level->wall_list, free);
	gfc_list_delete(level->wall_list);

	gfc_list_foreach(level->platform_list, free);
	gfc_list_delete(level->platform_list);

	free(level);
}

void level_update() {
	int i;
	//float offset;
	Ground* ground;
	Platform* plat;

	//offset = 1.0f;

	// draw ground
	for (i = 0; i < level_manager.curr_level->ground_list->count; i++) {
		ground = (Ground*)gfc_list_nth(level_manager.curr_level->ground_list, i);
		if (!ground) continue;

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

	// draw platforms
	update_platforms();
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

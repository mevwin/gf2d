#include "simple_logger.h"
#include "gf2d_draw.h"
#include "gfc_types.h"
#include "world.h"
#include "player_attack.h"
#include "collisions.h"
#include "item.h"
#include "enemy.h"

typedef struct LevelManager_S {
	Level*			curr_level;
	Uint32			curr_level_index;
	GFC_List*		level_list;				// a list of filepaths for levels
	Uint8			room_num;
	Uint8			respawning;
}LevelManager;

// NOTE: ONLY ONE LEVEL LOADED AT A TIME
// NOTE: ALL OF THE LEVEL'S ROOMS ARE LOADED IN MEMORY

/**
* SWAPPING BETWEEN ROOMS:
*	- save item data
*	- save enemy data
*	- change player position
*/

static LevelManager level_manager = { 0 };

void level_manager_close();

void create_room(Room* room, SJson* data);
void create_ground(Ground* ground, SJson* ground_data);
void create_wall(Wall* wall, GFC_Edge2D dimen, Uint8 type);
void create_platform(Platform* platform, SJson* plat_data);

void load_current_room();
void change_rooms(Uint8 new_index);

void update_platforms();
void move_platform(Platform* plat);

void level_manager_init(const char* filename){	
	SJson* level_def, *level_list;
	GFC_TextLine *path;
	int i;
	
	// load level list
	level_manager.level_list = gfc_list_new();
	level_def = sj_load(filename);
	level_list = sj_object_get_value(level_def, "list");

	for (i = 0; i < level_list->v.array->count; i++) {
		path = gfc_allocate_array(sizeof(GFC_TextLine), 1);
		gfc_line_cpy(path, sj_object_get_string(sj_array_get_nth(level_list, i), "path"));
		gfc_list_append(level_manager.level_list, path);
	}
	level_manager.curr_level_index = 0;
	level_manager.curr_level = NULL;

	// TODO: retrieve level objective descriptions and store as list

	sj_free(level_def);
	atexit(level_manager_close);
}

void level_manager_close() {
	int i;

	if (level_manager.curr_level) level_curr_close();

	for (i = 0; i < level_manager.level_list->count; i++) {
		free(gfc_list_nth(level_manager.level_list, i));
	}

	gfc_list_delete(level_manager.level_list);
	
	memset(&level_manager, 0, sizeof(LevelManager));
}

void level_load(Uint8 index) {
	Level* level;
	SJson *level_obj, *level_data, *layout, *row, *column;
	GFC_Vector2D layout_dimen;
	GFC_TextLine room_path;
	int i, j, check;

	level = gfc_allocate_array(sizeof(Level), 1);
	if (!level) {
		slog("failed to allocate space for level");
		return;
	}

	level_obj = sj_load(gfc_list_nth(level_manager.level_list, index));
	if (!level_obj) {
		slog("level not found");
		change_world_state(WORLD_CLOSE);
		return;
	}

	level_data = sj_object_get_value(level_obj, "level");

	// get name
	gfc_word_cpy(level->name, sj_object_get_string(level_data, "name"));

	// get type
	sj_object_get_value_as_int(level_data, "type", &i);
	level->level_type = (LvlType) i;

	// get level objective
	sj_object_get_value_as_int(level_data, "obj", &i);
	level->objective = (LvlObjective)i;

	/* generate room layout */
	sj_object_get_value_as_uint8(level_data, "start_room", &level->start_room);
	level_manager.room_num = level->start_room;

	sj_object_get_value_as_uint8(level_data, "room_count", &level->room_count);
	level->rooms = gfc_allocate_array(sizeof(Room), level->room_count);

	// load all the rooms
	// TODO: add room transitions
	sj_object_get_vector2d(level_data, "layout_dimen", &layout_dimen);
	layout = sj_object_get_value(level_data, "layout");
	for (i = 0; i < layout->v.array->count; i++) {
		row = sj_array_get_nth(layout, i);
		if (!row) continue;

		for (j = 0; j < row->v.array->count; j++) {
			column = sj_array_get_nth(row, j);
			if (!column) continue;		

			sj_get_integer_value(column, &check);
			if (!check) continue;

			// create room
			gfc_line_sprintf(room_path, "levels/%s/room%d.def", level->name, check);
			create_room(&level->rooms[check - 1], sj_load(room_path));
		}
	}
	

	/*

	// level objective
	sj_object_get_uint8(level_data, "obj", &i);
	level->obj = (LevelObjective)i;
	switch (level->obj) {
		case LEVEL_OBJ_SURVIVE:

			break;

		case LEVEL_OBJ_COLLECT:

			break;

		default: //LEVEL_OBJ_KILL_ALL_ENEMIES
			list = sj_object_get_value(sj_object_get_value(level_data, "enemies"), "list");
			level->goal = 0;
			level->goal_counter = list->v.array->count;
	}
	
	// items (that spawn in the level)
	list = sj_object_get_value(level_data, "items");
	if (!list) {
		slog("no item list");
		sj_free(level_obj);
		free(level);
		return;
	}
	for (i = 0; i < list->v.array->count; i++) {
		data = sj_array_get_nth(list, i);
		if (!data) continue;

		sj_object_get_vector2d(data, "position", &spawn);

		item_spawn(sj_object_get_string(data, "item_name"), spawn);
	}
	*/
	level_manager.curr_level = level;
	sj_free(level_obj);
}

void create_room(Room* room, SJson* data) {
	SJson* room_data, *list, *entry;
	int i;

	if (!room || !data) return;

	room_data = sj_object_get_value(data, "room");

	gfc_word_cpy(room->name, sj_object_get_string(room_data, "name"));
	sj_object_get_vector2d(room_data, "player_spawn", &room->player_spawn);

	// ground list
	list = sj_object_get_value(room_data, "ground_list");
	if (!list) {
		slog("no ground list");
		sj_free(data);
		return;
	}
	
	room->ground_count = list->v.array->count;
	room->grounds = gfc_allocate_array(sizeof(Ground), room->ground_count);
	for (i = 0; i < room->ground_count; i++) {
		entry = sj_array_get_nth(list, i);
		if (!entry) continue;

		create_ground(&room->grounds[i], entry);
	}

	// platform list
	list = sj_object_get_value(room_data, "platform_list");
	if (!list) {
		slog("no platform list");
		sj_free(data);
		free(room->grounds);
		return;
	}

	room->platform_count = list->v.array->count;
	room->platforms = gfc_allocate_array(sizeof(Platform), room->platform_count);
	for (i = 0; i < room->platform_count; i++) {
		entry = sj_array_get_nth(list, i);
		if (!entry) continue;

		create_platform(&room->platforms[i], entry);
	}

	/* initialize entity spawns */
	list = sj_object_get_value(room_data, "enemy_list");
	// enemy spawns
	enemy = sj_object_get_value(level_data, "enemies");
	list = sj_object_get_value(enemy, "list");
	if (!list) {
		slog("no enemy list");
		sj_free(level_obj);
		free(level);
		return;
	}
	for (i = 0; i < list->v.array->count; i++) {
		data = sj_array_nth(list, i);
		if (!data) {
			slog("no enemy found");
			continue;
		}
		sj_object_get_int(data, "type", &type);
		sj_object_get_vector2d(data, "spawn_position", &spawn);

		// check if enemy spawns at random position
		if (spawn.x == -1.0f && spawn.y == -1.0f) {
			data = sj_object_get_value(enemy, "random_spawns");
			random = gfc_random_int(data->v.array->count);
			sj_object_get_vector2d(sj_array_nth(data, random), "position", &spawn);
		}
		if (type == SPROUTER)
			sprouter_spawns = sj_object_get_value(enemy, "sprouter_spawns");
		else
			sprouter_spawns = NULL;

		enemy_spawn(type, spawn, sprouter_spawns);
	}

	sj_free(data);
}

void create_ground(Ground* ground, SJson* ground_data) {
	GFC_Vector4D rec_buf;

	if (!ground || !ground_data)
		return;

	sj_object_get_vector4d(ground_data, "rect", &rec_buf);
	ground->dimensions = gfc_rect_from_vector4(rec_buf);
	ground->region = gfc_vector2d(ground->dimensions.x,
								ground->dimensions.x + ground->dimensions.w);

	ground->color = sj_object_get_color(ground_data, "color"); 	// TODO: REMOVE LATER

	/* initalize walls */
	sj_object_get_uint8(ground_data, "walls", &ground->wall_flag);
	
	// create left wall
	create_wall(&ground->walls[0], get_edge_from_rect(ground->dimensions, 3), 0);

	// create right wall
	create_wall(&ground->walls[1], get_edge_from_rect(ground->dimensions, 2), 1);
	
	sj_object_get_uint8(ground_data, "ceiling", &ground->ceil_flag);

	return ground;
}

void create_wall(Wall* wall, GFC_Edge2D dimen, Uint8 type) {
	wall = gfc_allocate_array(sizeof(Wall), 1);
	if (!wall) {
		slog("failed to allocate memory for wall");
		return;
	}

	wall->dimensions = dimen;
	wall->type = type;
	//wall->wjumpable = 1;
}

void create_platform(Platform* platform, SJson* plat_data){
	GFC_Vector4D rec_buf;
	GFC_Rect dimen;
	Uint8 i;

	if (!plat_data) {
		slog("failed to allocate memory for platform");
		return;
	}

	sj_object_get_vector4d(plat_data, "rect", &rec_buf);
	dimen = gfc_rect_from_vector4(rec_buf);
	platform->dimensions = dimen;
	platform->region = gfc_vector2d(platform->dimensions.x,
									platform->dimensions.x + platform->dimensions.w);

	sj_object_get_uint8(plat_data, "pass_through", &platform->pass_through);

	sj_object_get_uint8(plat_data, "starting_move", &i);
	platform->moveType = (PlatformMove)i;

	sj_object_get_uint8(plat_data, "moving", &platform->moving);
	if (platform->moving) {
		sj_object_get_vector2d(plat_data, "move_speed", &platform->move_speed);
		sj_object_get_vector4d(plat_data, "move_bounds", &platform->move_bounds);
	}
}

void update_platforms() {
	Platform* plat;
	Room* room;
	int i;

	room = get_current_room();
	for (i = 0; i < room->platform_count; i++) {
		plat = &room->platforms[i];
		if (!plat) continue;

		// draw plat
		gf2d_draw_rect_filled(plat->dimensions, GFC_COLOR_BLACK);

		// move platform if allowed
		if (plat->moving)
			move_platform(plat);
	}
}

// TODO: CHANGE BEHAVIOR LATER
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

		case PLATFORM_MOVE_UP:
			plat->dimensions.y -= plat->move_speed.y;
			if (plat->dimensions.y <= plat->move_bounds.z)
				plat->moveType = PLATFORM_MOVE_DOWN;

			break;

		case PLATFORM_MOVE_DOWN:
			plat->dimensions.y += plat->move_speed.y;
			if (plat->dimensions.y + plat->dimensions.h >= plat->move_bounds.w)
				plat->moveType = PLATFORM_MOVE_UP;

			break;
	}

	// update region
	plat->region = gfc_vector2d(plat->dimensions.x,
								plat->dimensions.x + plat->dimensions.w);
}

void level_curr_close() {
	Room* room;

	room = get_current_room();

	free(room->grounds);
	free(room->platforms);

	memset(level_manager.curr_level, 0, sizeof(Level));
}

/** 
 * optimizing draw calls for static surfaces:
 * initialize a surface:
 * 		- for each piece of ground
 * 			- gf2d_sprite_draw_to_surface
 * 		- create sprite from surface
 * 		- gf2d_sprite_load_all
 * 
 * 
*/

/*
void draw_ground_to_surface(GFC_List* ground_list){
	SDL_Surface *surface;
	Ground* ground;
	GFC_Vector2D position;
	int i;
	
	if (!ground_list) return;

	surface = gf2d_graphics_create_surface(RES.x + RES.w, RES.y + RES.h);

	for (i = 0; ground_list->count; i++){
		ground = (Ground*) gfc_list_nth(ground_list, i);
		if (!ground) continue;

		position = gfc_vector2d(ground->dimensions.x, ground->dimensions.y);
		gf2d_sprite_draw_to_surface(sprite, position, NULL, NULL, 1, surface);
	}
}
*/

void load_current_room() {
	
}

void change_rooms(Uint8 new_index) {
	level_manager.room_num = new_index;
	load_current_room();
}

void level_update() {
	int i;
	Ground* ground;
	Room* room;
	GFC_List* enemy_list;

	room = get_current_room();
	
	// draw ground
	for (i = 0; i < room->ground_count; i++) {
		ground = &room->grounds[i];
		if (!ground) continue;

		gf2d_draw_rect_filled(ground->dimensions, ground->color);
	}

	// draw platforms
	update_platforms();

	/*
	// check if level goal has been reached
	switch (level_manager.curr_level->obj) {
		case LEVEL_OBJ_SURVIVE:

				break;

		case LEVEL_OBJ_COLLECT:

			break;

		default: //LEVEL_OBJ_KILL_ALL_ENEMIES
			enemy_list = get_enemy_list();
			level_manager.curr_level->goal_counter = enemy_list->count;
	}
	
	if (level_manager.curr_level->goal == level_manager.curr_level->goal_counter) {
		level_manager.curr_level_index++;
		if (level_manager.curr_level_index == level_manager.level_list->count)
			change_world_state(WORLD_GAME_COMPLETE);
		else
			change_world_state(WORLD_LEVELCOMPLETE);
	}
	*/
}

/*
void level_camera_update(GFC_Vector2D move_speed) {
	Ground* ground;
	int i;

	for (i = 0; i < level_manager.curr_level->ground_list->count; i++) {
		ground = (Ground*)gfc_list_nth(level_manager.curr_level->ground_list, i);
		if (!ground) continue;

		if (move_speed.x != 0)
			ground->dimensions.x -= move_speed.x;

		//if (ground->dimensions.x < 0)
			
		//ground->dimensions.y += move_speed.y;

		// update region
		ground->region = gfc_vector2d(ground->dimensions.x,
										ground->dimensions.x + ground->dimensions.w);
	}
}
*/

void load_next_level(void* p){
	Entity* player;
	PlayerData* p_data;

	player = (Entity*) p;
	if (!player) {
		slog("player is null");
		change_world_state(WORLD_CLOSE);
		return;
	}
	p_data = (PlayerData*)player->data;

	level_load(level_manager.curr_level_index);

	//reset player stuff
	player_atk_system_reset();
	p_data->canMove = 1;
	p_data->isAttacking = 0;
	//gfc_vector2d_copy(player->position, level_manager.curr_level->player_spawn);
	//gfc_vector2d_copy(p_data->spawn_pos, level_manager.curr_level->player_spawn);
}

void restart_level(void* p) {
	Entity* player;
	PlayerData* p_data;

	player = (Entity*)p;
	if (!player) {
		slog("player is null");
		change_world_state(WORLD_CLOSE);
		return;
	}
	p_data = (PlayerData*)player->data;

	level_load(level_manager.curr_level_index);

	//reset player stuff
	player_atk_system_reset();
	player_respawn(player, player->data);
	//gfc_vector2d_copy(player->position, level_manager.curr_level->player_spawn);
	//gfc_vector2d_copy(p_data->spawn_pos, level_manager.curr_level->player_spawn);
	
}

Level* get_curr_level() {
	return level_manager.curr_level;
}

Uint8 entity_keep_in_bounds(void* e, Uint8 edge_type) {
	GFC_Edge2D screen_edge, e_edge;
	Entity* self;
	//GFC_Vector2D p1, p2;
	float offset;

	//return 0; // temporary

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

Room* get_current_room() {
	return &level_manager.curr_level->rooms[level_manager.room_num];
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

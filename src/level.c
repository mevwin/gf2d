#include "simple_logger.h"
#include "gf2d_draw.h"
#include "world.h"
#include "level.h"
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
GFC_Rect level_find_nearest_ground(Entity* ent);
GFC_Edge2D level_find_nearest_ceiling(Entity* ent, GFC_Edge2D e_top);
Wall* level_find_nearest_wall(Entity* ent, Uint8 wall_type);

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

GFC_Rect level_find_nearest_ground(Entity* ent) {
	Ground* ground;
	GFC_Edge2D left, right, g_level, g_bottom;
	int i;

	left = get_edge_from_rect(ent->boundbox.s.r, 3);
	right = get_edge_from_rect(ent->boundbox.s.r, 2);

	for (i = 0; i < level_manager.curr_level->ground_list->count; i++) {
		ground = (Ground*) gfc_list_nth(level_manager.curr_level->ground_list, i);
		
		if (!ground) continue;

		g_level = get_edge_from_rect(ground->dimensions, 1);
		g_bottom = get_edge_from_rect(ground->dimensions, 0);

		if (right.x1 >= ground->region.x && left.x1 < ground->region.y
			&& g_level.y1 - (left.y2 - ent->velocity.y) <= 7.0f
			&& g_bottom.y1 > (left.y2 - ent->velocity.y)
			) {
			// slog("%f", g_level.y1 - (left.y2 - ent->velocity.y));
			return ground->dimensions;
		}
	}
	return gfc_rect(-20.0f, 1000.0f, 1800.0f, 200.0f);
}

Uint8 ground_collision(void* ent) {
	Entity* self;
	PlayerData* p_data;
	GFC_Edge2D edge, bottom;
	GFC_Rect ground;

	self = (Entity*) ent;
	if (!self) {
		slog("no entity");
		return 0;
	}

	ground = level_find_nearest_ground(self);
	if (ground.x == -20.f)
		return 0;
	
	edge = get_edge_from_rect(ground, 1);

	bottom = get_edge_from_rect(self->boundbox.s.r, 0); // player's bottom edge
	bottom.y1 += -self->velocity.y;

	if (roundf(bottom.y1) == edge.y1) //touching ground
		return 2;
	else if (bottom.y1 >= edge.y1 - 1.0f) { // about to touch ground
		self->position.y = edge.y1 - self->boundbox.s.r.h / 2.0f; // check to make sure not to clip through ground
		return 1;
	}
	else { // falling
		if (self->type == PLAYER) {
			p_data = self->data;
			if (!p_data) return 0;

			// increment jump counter by 1 to avoid the MultiVersus DJ
			if (!p_data->jump_count)
				p_data->jump_count++;

			//slog("player: %f, ground: %f", bottom.y1, edge.y1);
		}
		else if (self->type == ENEMY) {
			// TODO
		}

		return 0;
	}
}

GFC_Edge2D level_find_nearest_ceiling(Entity* ent, GFC_Edge2D e_top) {
	Ground* ground;
	GFC_Vector2D ceil_point, e_point;
	GFC_Edge2D g_bottom;
	//GFC_Color color;
	int i;
	float offset;

	offset = 100.0f;

	for (i = 0; i < level_manager.curr_level->ground_list->count; i++) {
		ground = (Ground*) gfc_list_nth(level_manager.curr_level->ground_list, i);

		if (!ground || !ground->ceil_flag) { 
			//slog("%i", i);
			continue; 
		}

		g_bottom = get_edge_from_rect(ground->dimensions, 0);

		if (e_top.x2 > g_bottom.x1 && e_top.x1 < g_bottom.x2) {
			ceil_point = gfc_vector2d(ent->position.x, g_bottom.y1);
			e_point = gfc_vector2d(ent->position.x, e_top.y1);
			//e_point.y += ent->velocity.y;

			//color = GFC_COLOR_BLUE;
			//gf2d_draw_line(ceil_point, e_point, color);
			if (gfc_vector2d_distance_between_less_than(ceil_point, e_point, offset))
				return g_bottom;
		}
	}
	return gfc_edge(-20.0f, 1000.0f, 1800.0f, 200.0f);
}

Uint8 ceiling_collision(void* ent) {
	Entity* self;
	PlayerData* p_data;
	GFC_Edge2D e_top, ceil;
	float offset;

	self = (Entity*)ent;
	if (!self) {
		slog("no entity");
		return 0;
	}

	e_top = get_edge_from_rect(self->boundbox.s.r, 1);
	e_top.y1 += self->velocity.y;

	ceil = level_find_nearest_ceiling(self, e_top);
	if (ceil.x1 == -20.f)
		return 0;

	offset = 1.0f;

	if (roundf(ceil.y1) == e_top.y1) //touching ceiling
		return 2;
	else if (e_top.y1 < ceil.y1 + offset) { // about to touch ceiling
		self->position.y = ceil.y1 + self->boundbox.s.r.h / 2.0f + (2.0f * offset); // check to make sure not to clip through ground
		//self->velocity.y = 0;
		return 1;
	}
	else { // falling
		if (self->type == PLAYER) {
			//self->velocity.y = 0;

			p_data = self->data;
			if (!p_data) return 0;
			
			p_data->moveTypeY = FALLING;
		}
		else if (self->type == ENEMY) {
			// TODO
		}

		return 0;
	}
}

Wall* level_find_nearest_wall(Entity* ent, Uint8 wall_type) {
	Wall* wall;
	GFC_Vector2D wall_point, p_point; //, p1, p2;
	GFC_Edge2D p_side;
	int i;
	float offset;

	offset = 20.0f;

	if (!ent) {
		slog("no entity given");
		return;
	}

	for (i = 0; i < level_manager.curr_level->wall_list->count; i++) {
		wall = (Wall*)gfc_list_nth(level_manager.curr_level->wall_list, i);

		if (!wall) continue;

		p_side = get_edge_from_rect(ent->boundbox.s.r, 2);

		if (p_side.y2 > wall->dimensions.y1 && p_side.y1 < wall->dimensions.y2) {
			wall_point = gfc_vector2d(wall->dimensions.x1, ent->position.y);
			p_side = get_edge_from_rect(ent->boundbox.s.r, wall_type + 2);
			p_point = gfc_vector2d(p_side.x1, ent->position.y);
			p_point.x += wall_type ? ent->velocity.x : -ent->velocity.x;

			//color = wall_type ? GFC_COLOR_BLUE : GFC_COLOR_RED;
			//gf2d_draw_line(wall_point, p_point, color);

			if (gfc_vector2d_distance_between_less_than(wall_point, p_point, offset))
				return wall;
		}
	}

	return NULL; // if no wall is found
}

Uint8 wall_collision(void* ent, Uint8 wall_type) {
	Entity* self;
	Wall* wall;
	GFC_Edge2D p_side, p_top;
	Uint8 side_type;
	float offset;

	self = (Entity*) ent;
	if (!ent) {
		slog("no entity given");
		return;
	}

	wall = level_find_nearest_wall(self, wall_type);
	if (!wall){
		//slog("no wall found");
		return 0;
	}

	side_type = wall_type ? 3 : 2; 
	p_side = get_edge_from_rect(self->boundbox.s.r, side_type);
	p_top = get_edge_from_rect(self->boundbox.s.r, 0);
	offset = 2.0f;

	//if (roundf(p_side.x1) == wall->dimensions.x1) { // touching wall
		//slog("hugging wall");
		//return 2;
	//}
	if ((wall_type && p_side.x1 - self->velocity.x <= wall->dimensions.x1 + offset) || // right side
		(!wall_type && p_side.x1 + self->velocity.x >= wall->dimensions.x1 - offset)) {// left side
		//slog("about to touch left wall");
		return 1;
	}
	else {
		//slog("not touching wall");
		return 0;
	}
}

GFC_Edge2D get_edge_from_rect(GFC_Rect box, Uint8 side) {
	GFC_Edge2D edge;

	switch (side) {
		case 3: // left
			edge = gfc_edge_from_vectors(
				gfc_vector2d(box.x, box.y),
				gfc_vector2d(box.x, box.y + box.h)
			);

			break;

		case 2: // right
			edge = gfc_edge_from_vectors(
				gfc_vector2d(box.x + box.w, box.y),
				gfc_vector2d(box.x + box.w, box.y + box.h)
			);

			break;

		case 1: // top
			edge = gfc_edge_from_vectors(
				gfc_vector2d(box.x, box.y),
				gfc_vector2d(box.x + box.w, box.y)
			);

			break;

		default: // bottom
			edge = gfc_edge_from_vectors(
				gfc_vector2d(box.x, box.y + box.h),
				gfc_vector2d(box.x + box.w, box.y + box.h)
			);
	}
	return edge;
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

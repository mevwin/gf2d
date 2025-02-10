#include "simple_logger.h"
#include "gf2d_draw.h"
#include "world.h"
#include "level.h"
#include "player.h"
#include "enemy.h"

typedef struct LevelManager_S {
	Level*			curr_level;
	GFC_List*		level_list;
	Uint8			level_num;
	Uint8			w_collision;  // 0 for left, 1 for right
}LevelManager;

static LevelManager level_manager = { 0 };

void level_manager_close();
Ground* create_ground(GFC_Rect dimen, GFC_Color color, Uint8 walls);
GFC_Rect level_find_nearest_ground(Entity* ent);
Wall* level_find_nearest_wall(Entity* ent, Uint8 wall_type);

void level_manager_init() {
	level_manager.level_list = gfc_list_new();

	// load first level
	level_manager.curr_level = level_load(0);
	if (!level_manager.curr_level) {
		slog("failed to initialiaze first level");
		world_done_change();
		return;
	}

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

Ground* create_ground(GFC_Rect dimen, GFC_Color color, Uint8 walls) {
	Ground* ground;

	ground = gfc_allocate_array(sizeof(Ground), 1);
	if (!ground)
		return NULL;

	ground->dimensions = dimen;
	ground->region = gfc_vector2d(ground->dimensions.x,
								  ground->dimensions.x + ground->dimensions.w);
	ground->color = color;

	if (walls) { // initialize wall data
		ground->wall_flag = 1;
		ground->wall_left = gfc_allocate_array(sizeof(Wall), 1);
		ground->wall_left->dimensions = get_edge_from_rect(ground->dimensions, 3);
		ground->wall_left->type = 0;

		ground->wall_right = gfc_allocate_array(sizeof(Wall), 1);
		ground->wall_right->dimensions = get_edge_from_rect(ground->dimensions, 2);
		ground->wall_right->type = 1;
	}
	return ground;
}

Level* level_load(Uint8 index) {
	Level* level;
	Ground* ground1, *ground2;

	level = gfc_allocate_array(sizeof(Level), 1);
	if (!level) {
		slog("failed to initialize level");
		return NULL;
	}

	level->ground_list = gfc_list_new();
	
	ground1 = create_ground(gfc_rect(0, 600, 1200, 120), GFC_COLOR_BLACK, 0);
	gfc_list_append(level->ground_list, ground1);

	ground2 = create_ground(gfc_rect(200, 400, 500, 320), GFC_COLOR_YELLOW, 1);
	gfc_list_append(level->ground_list, ground2);

	//level->curr_ground = gfc_list_nth(level->ground_list, 0);

	level->player_spawn = gfc_vector2d(600, 200);

	enemy_spawn(BRUISER, gfc_vector2d(300, 200));

	return level;
}

void level_close(Level* level) {
	if (!level) {
		slog("no level to close");
		return;
	}

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

GFC_Rect level_find_nearest_ground(Entity* ent) {
	Ground* ground;
	GFC_Edge2D left, right, g_level;
	int i;

	left = get_edge_from_rect(ent->boundbox.s.r, 3);
	right = get_edge_from_rect(ent->boundbox.s.r, 2);

	for (i = 0; i < level_manager.curr_level->ground_list->count; i++) {
		ground = (Ground*) gfc_list_nth(level_manager.curr_level->ground_list, i);
		
		if (!ground) continue;

		g_level = get_edge_from_rect(ground->dimensions, 1);

		if (right.x1 >= ground->region.x && left.x1 <= ground->region.y 
			&& g_level.y1 - left.y2 <= 2.0f)
			return ground->dimensions;
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

	bottom = get_edge_from_rect(self->boundbox.s.r, 0); // player's bottom edge
	ground = level_find_nearest_ground(self);
	edge = get_edge_from_rect(ground, 1);

	if (roundf(bottom.y1) == edge.y1) //touching ground
		return 2;
	else if (bottom.y1 - self->velocity.y > edge.y1 - 1.0f) { // about to touch ground
		self->position.y = edge.y1 - self->boundbox.s.r.h / 2.0f; // check to make sure not to clip through ground
		return 1;
	}
	else {
		p_data = self->data;
		if (!p_data) return 0;
		
		// increment jump counter by 1 to avoid the MultiVersus DJ
		if (!p_data->jump_count)
			p_data->jump_count++;

		return 0;
	}
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
	offset = 1.0f;

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


Wall* level_find_nearest_wall(Entity* ent, Uint8 wall_type) {
	Ground* ground;
	Wall* wall;
	GFC_Vector2D wall_point, p_point; //, p1, p2;
	GFC_Edge2D p_side, p_bottom;
	GFC_Color color;
	int i;
	float offset;

	offset = 30.0f;

	if (!ent) {
		slog("no entity given");
		return;
	}
	
	for (i = 0; i < level_manager.curr_level->ground_list->count; i++) {
		ground = (Ground*) gfc_list_nth(level_manager.curr_level->ground_list, i);

		if (!ground->wall_flag) continue; 
		
		p_bottom = get_edge_from_rect(ent->boundbox.s.r, 0);
		//p_top = get_edge_from_rect(ent->boundbox.s.r, 1);

		wall = wall_type ? ground->wall_right : ground->wall_left;

		//p1 = gfc_vector2d(wall->dimensions.x1, 0);
		//p2 = gfc_vector2d(wall->dimensions.x2, 1200);
		//gf2d_draw_line(p1, p2, GFC_COLOR_GREEN);

		if (p_bottom.y1 > wall->dimensions.y1) { // && p_top.y1 <= wall->dimensions.y2
			wall_point = gfc_vector2d(wall->dimensions.x1, ent->position.y);
			p_side = get_edge_from_rect(ent->boundbox.s.r, wall_type+2);
			p_point = gfc_vector2d(p_side.x1, ent->position.y);
			p_point.x += wall_type ? ent->velocity.x : -ent->velocity.x;

			color = wall_type ? GFC_COLOR_BLUE : GFC_COLOR_RED;

			//gf2d_draw_line(wall_point, p_point, color);
			if (gfc_vector2d_distance_between_less_than(wall_point, p_point, offset))
				return wall;
		}
	}


	return NULL; // if no wall is found
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

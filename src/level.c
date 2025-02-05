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
	Uint8			w_collision;  // 0 for left, 1 for right
}LevelManager;

static LevelManager level_manager = { 0 };

void level_manager_close();
Ground* create_ground(GFC_Rect dimen, GFC_Color color, Uint8 walls);
void level_find_nearest_ground();
GFC_Edge2D level_find_nearest_wall(Entity* ent);

void level_manager_init() {
	level_manager.level_list = gfc_list_new();

	// load first level
	level_manager.curr_level = level_load(0);
	if (!level_manager.curr_level) {
		slog("failed to initialiaze first level");
		world_done_change();
		level_manager_close();
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
		slog("made a ground w/ walls");
		ground->wall_flag = 1;
		ground->walls.left = get_edge_from_rect(ground->dimensions, 3);
		ground->walls.right = get_edge_from_rect(ground->dimensions, 2);
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
	
	ground1 = create_ground(gfc_rect(400, 600, 800, 120), GFC_COLOR_BLACK, 0);
	gfc_list_append(level->ground_list, ground1);

	ground2 = create_ground(gfc_rect(0, 400, 400, 320), GFC_COLOR_YELLOW, 1);
	gfc_list_append(level->ground_list, ground2);

	level->curr_ground = gfc_list_nth(level->ground_list, 0);

	level->player_spawn = gfc_vector2d(600, 200);

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
			return;
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

	bottom = get_edge_from_rect(self->boundbox.s.r, 0); // player's bottom edge
	level_find_nearest_ground();

	edge = get_edge_from_rect(level_manager.curr_level->curr_ground->dimensions, 1);

	if (roundf(bottom.y1) >= edge.y1 - 1.0f) {
		return 1;
	}
	else return 0;

	//return gfc_edge_intersect(bottom, edge);
}

GFC_Edge2D level_find_nearest_wall(Entity* ent) {
	Ground* ground;
	GFC_Vector2D wall_point, p_point;
	GFC_Edge2D p_side, p_bottom, p_top;
	int i;
	float offset;

	offset = 20.0f;
	
	for (i = 0; i < level_manager.curr_level->ground_list->count; i++) {
		ground = (Ground*) gfc_list_nth(level_manager.curr_level->ground_list, i);

		if (!ground->wall_flag) continue; 
		
		p_bottom = get_edge_from_rect(ent->boundbox.s.r, 0);
		p_top = get_edge_from_rect(ent->boundbox.s.r, 1);
		
		// check left wall first
		if (p_bottom.y1 >= ground->walls.left.y1 && p_top.y1 <= ground->walls.left.y2) {
			wall_point = gfc_vector2d(ground->walls.left.x1, ent->position.y);
			p_side = get_edge_from_rect(ent->boundbox.s.r, 2);
			p_point = gfc_vector2d(p_side.x1, ent->position.y);
			p_point.x += ent->velocity.x;

			if (gfc_vector2d_distance_between_less_than(wall_point, p_point, offset)) {
				level_manager.w_collision = 0;
				return ground->walls.left;
			}
		}

		// check right wall
		if (ent->position.y >= ground->walls.right.y1) {
			wall_point = gfc_vector2d(ground->walls.right.x1, ent->position.y);
			p_side = get_edge_from_rect(ent->boundbox.s.r, 3);
			p_point = gfc_vector2d(p_side.x1, ent->position.y);
			p_point.x -= ent->velocity.x;

			gf2d_draw_line(wall_point, p_point, GFC_COLOR_BLUE);
			if (gfc_vector2d_distance_between_less_than(wall_point, p_point, offset)) {
				level_manager.w_collision = 1;
				return ground->walls.right;
			}
		}
	}

	//("false");
	return gfc_edge(-1, -1, -1, -1); // if no wall is found, return dummy edge
}

Uint8 wall_collision(void* ent) {
	Entity* self;
	GFC_Edge2D nearest, e_side;
	Uint8 wall_type;
	int i;
	float offset;

	self = (Entity*)ent;
	if (!self) {
		slog("no entity");
		return 0;
	}

	nearest = level_find_nearest_wall(self);
	if (nearest.x1 == -1) { // no wall found
		return 0;
	}
	else { // wall found, see if player is hitting it
		offset = 3.0f;
		
		wall_type = level_manager.w_collision ? 3 : 2;
		e_side = get_edge_from_rect(self->boundbox.s.r, wall_type);

		if (self->velocity.x > 0)
			e_side.x1 += wall_type == 3 ? self->velocity.x : -self->velocity.x;
		
		//slog("wall %f : player %f", nearest.x1, e_side.x1);

		if (wall_type == 2 && roundf(e_side.x1) >= nearest.x1 - offset) {
			self->collis_repo = gfc_vector2d(self->position.x - offset, self->position.y);
			slog("left wall collision");
			return 1;
		}
		else if (wall_type == 3 && roundf(e_side.x1) <= nearest.x1 + offset) {
			self->collis_repo = gfc_vector2d(self->position.x + offset, self->position.y);
			slog("right wall collision");
			return 1;
		}
		else {
			//slog("wall found but no collision");
			return 0;
		}
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
				gfc_vector2d(box.x + box.w, box.y)
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
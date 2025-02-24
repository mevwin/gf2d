#include "simple_logger.h"
#include "world.h"
#include "collisions.h"
#include "player.h"
#include "enemy.h"

GFC_Rect level_find_nearest_ground(Entity* ent);
Platform* level_find_nearest_plat(Entity* ent);
GFC_Edge2D level_find_nearest_ceiling(Entity* ent, GFC_Edge2D e_top);
Wall* level_find_nearest_wall(Entity* ent, WallType wall_type);

GFC_Rect level_find_nearest_ground(Entity* ent) {
	Ground* ground;
	GFC_Edge2D left, right, g_level, g_bottom;
	Level* level;
	int i;

	left = get_edge_from_rect(ent->boundbox.s.r, 3);
	right = get_edge_from_rect(ent->boundbox.s.r, 2);

	level = get_curr_level();

	for (i = 0; i < level->ground_list->count; i++) {
		ground = (Ground*)gfc_list_nth(level->ground_list, i);

		if (!ground) continue;

		g_level = get_edge_from_rect(ground->dimensions, 1);
		g_bottom = get_edge_from_rect(ground->dimensions, 0);

		if (right.x1 >= ground->region.x && left.x1 < ground->region.y
			&& g_level.y1 - (left.y2 - ent->velocity.y) <= 7.0f
			&& g_bottom.y1 >(left.y2 - ent->velocity.y)
			) {
			// slog("%f", g_level.y1 - (left.y2 - ent->velocity.y));
			return ground->dimensions;
		}
	}
	return gfc_rect(-20.0f, 1000.0f, 1800.0f, 200.0f);
}

Uint8 ground_collision(void* ent) {
	Entity* self;
	GFC_Edge2D g_top, e_bottom;
	GFC_Rect ground;
	float offset;

	self = (Entity*)ent;
	if (!self) {
		slog("no entity");
		return 0;
	}

	ground = level_find_nearest_ground(self);
	if (ground.x == -20.f)
		return 0;
	
	offset = 1.0f;

	g_top = get_edge_from_rect(ground, 1);

	e_bottom = get_edge_from_rect(self->boundbox.s.r, 0); // player's bottom edge
	e_bottom.y1 += -self->velocity.y;

	if (e_bottom.y1 >= g_top.y1 - offset) { // about to touch ground
		self->position.y = g_top.y1 - self->boundbox.s.r.h / 2.0f - offset; // check to make sure not to clip through ground
		return 1;
	}
	else return 0;
}

Platform* level_find_nearest_plat(Entity* ent) {
	Platform* plat;
	GFC_Edge2D e_left, e_right, p_top, p_bottom;
	Level* level;
	int i;

	e_left = get_edge_from_rect(ent->boundbox.s.r, 3);
	e_right = get_edge_from_rect(ent->boundbox.s.r, 2);
	level = get_curr_level();

	for (i = 0; i < level->platform_list->count; i++) {
		plat = (Platform*)gfc_list_nth(level->platform_list, i);

		if (!plat) continue;

		p_top = get_edge_from_rect(plat->dimensions, 1);
		p_bottom = get_edge_from_rect(plat->dimensions, 0);

		if (e_right.x1 >= plat->region.x && e_left.x1 < plat->region.y
			&& p_top.y1 - (e_left.y2 - ent->velocity.y) <= 7.0f
			&& p_bottom.y1 >(e_left.y2 - ent->velocity.y)
			) {
			// slog("%f", g_level.y1 - (left.y2 - ent->velocity.y));
			return plat;
		}
	}
	return NULL;
}

Uint8 platform_collision(void* ent) {
	Platform* plat;
	Entity* self;
	GFC_Edge2D e_bottom, plat_top;
	float offset;

	self = (Entity*)ent;
	if (!self) {
		slog("no entity");
		return 0;
	}
	
	plat = level_find_nearest_plat(self);
	if (!plat)
		return 0;

	offset = 1.0f;

	plat_top = get_edge_from_rect(plat->dimensions, 1);

	e_bottom = get_edge_from_rect(self->boundbox.s.r, 0); // player's bottom edge
	e_bottom.y1 -= self->velocity.y;

	if (!plat->pass_through)
		self->plat_flag = 0;

	if (self->plat_flag && e_bottom.y1 >= plat_top.y1 - offset && !gfc_rect_overlap(self->boundbox.s.r, plat->dimensions)) { // about to touch ground
		self->position.y = plat_top.y1 - self->boundbox.s.r.h / 2.0f - offset; // check to make sure not to clip through plat
		return 1;
	}
	else return 0;
}

void handle_ent_plat_collision(void* e) {
	Entity* ent;
	Platform* plat;
	GFC_Edge2D e_bottom, plat_top;
	float offset;

	ent = (Entity*) e;
	if (!ent) return;

	plat = level_find_nearest_plat(ent);
	if (!plat) return;

	offset = 1.0f;
	plat_top = get_edge_from_rect(plat->dimensions, 1);
	e_bottom = get_edge_from_rect(ent->boundbox.s.r, 0); // player's bottom edge
	e_bottom.y1 -= ent->velocity.y;

	// adjust player position if on moving platform
	if (plat->moving) {
		switch (plat->moveType) {
			case PLATFORM_MOVE_LEFT:
				ent->position.x -= plat->move_speed.x;

				break;

			case PLATFORM_MOVE_RIGHT:
				ent->position.x += plat->move_speed.x;

				break;
		}
	}

	//adjust player's y position to stay on top of platform
	ent->position.y = plat_top.y1 - ent->boundbox.s.r.h / 2.0f - offset;
}

GFC_Edge2D level_find_nearest_ceiling(Entity* ent, GFC_Edge2D e_top) {
	Ground* ground;
	GFC_Vector2D ceil_point, e_point;
	GFC_Edge2D g_bottom;
	int i;
	float offset;
	Level* level;

	level = get_curr_level();

	offset = 7.0f;

	for (i = 0; i < level->ground_list->count; i++) {
		ground = (Ground*)gfc_list_nth(level->ground_list, i);

		if (!ground || !ground->ceil_flag) continue;

		g_bottom = get_edge_from_rect(ground->dimensions, 0);

		if (e_top.x2 > g_bottom.x1 && e_top.x1 < g_bottom.x2) {
			ceil_point = gfc_vector2d(ent->position.x, g_bottom.y1);
			e_point = gfc_vector2d(ent->position.x, e_top.y1);

			if (gfc_vector2d_distance_between_less_than(ceil_point, e_point, offset))
				return g_bottom;
		}
	}
	return gfc_edge(-20.0f, 1000.0f, 1800.0f, 200.0f);
}

Uint8 ceiling_collision(void* ent) {
	Entity* self;
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

	if (e_top.y1 <= ceil.y1 + offset) { // about to touch ceiling
		self->position.y = ceil.y1 + (self->boundbox.s.r.h / 2.0f) + (2.0f*offset); // check to make sure not to clip through ground
		return 1;
	}
	else return 0;
}

Wall* level_find_nearest_wall(Entity* ent, WallType wall_type) {
	Wall* wall;
	GFC_Vector2D wall_point, p_point; //, p1, p2;
	GFC_Edge2D p_side;
	int i;
	float offset;
	Level* level;

	if (!ent) {
		slog("no entity given");
		return NULL;
	}

	offset = 20.0f;
	level = get_curr_level();

	for (i = 0; i < level->wall_list->count; i++) {
		wall = (Wall*)gfc_list_nth(level->wall_list, i);

		if (!wall) continue;

		p_side = get_edge_from_rect(ent->boundbox.s.r, 2);

		if (p_side.y2 > wall->dimensions.y1 && p_side.y1 < wall->dimensions.y2) {
			wall_point = gfc_vector2d(wall->dimensions.x1, ent->position.y);
			p_side = get_edge_from_rect(ent->boundbox.s.r, ((Uint8) wall_type) + 2);
			p_point = gfc_vector2d(p_side.x1, ent->position.y);
			p_point.x += wall_type == WALL_LEFT ? ent->velocity.x : -ent->velocity.x;

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

	self = (Entity*)ent;
	if (!ent) {
		slog("no entity given");
		return 0;
	}

	wall = level_find_nearest_wall(self, wall_type);
	if (!wall) {
		//slog("no wall found");
		return 0;
	}

	side_type = wall_type == WALL_RIGHT ? 3 : 2;
	p_side = get_edge_from_rect(self->boundbox.s.r, ((Uint8) side_type));
	p_top = get_edge_from_rect(self->boundbox.s.r, 0);
	offset = 2.0f;

	if ((wall_type == WALL_RIGHT && p_side.x1 - self->velocity.x <= wall->dimensions.x1 + offset) || // right side
		(wall_type == WALL_LEFT && p_side.x1 + self->velocity.x >= wall->dimensions.x1 - offset)) {// left side
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

Platform* get_colliding_plat(void* ent) {
	return level_find_nearest_plat(ent);
}

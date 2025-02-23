#include "simple_logger.h"
#include "world.h"
#include "collisions.h"
#include "player.h"
#include "enemy.h"

GFC_Rect level_find_nearest_ground(Entity* ent);
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
	PlayerData* p_data;
	GFC_Edge2D edge, bottom;
	GFC_Rect ground;
	Level* level;

	self = (Entity*)ent;
	if (!self) {
		slog("no entity");
		return 0;
	}

	ground = level_find_nearest_ground(self);
	if (ground.x == -20.f)
		return 0;

	level = get_curr_level();

	edge = get_edge_from_rect(ground, 1);

	bottom = get_edge_from_rect(self->boundbox.s.r, 0); // player's bottom edge
	bottom.y1 += -self->velocity.y;

	if (bottom.y1 >= edge.y1 - 1.0f) { // about to touch ground
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
	Level* level;

	level = get_curr_level();

	offset = 7.0f;

	for (i = 0; i < level->ground_list->count; i++) {
		ground = (Ground*)gfc_list_nth(level->ground_list, i);

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

	if (e_top.y1 <= ceil.y1 + offset) { // about to touch ceiling
		self->position.y = ceil.y1 + (self->boundbox.s.r.h / 2.0f) + (5.0f*offset); // check to make sure not to clip through ground
		//self->velocity.y = 0;
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

	//if (roundf(p_side.x1) == wall->dimensions.x1) { // touching wall
		//slog("hugging wall");
		//return 2;
	//}
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

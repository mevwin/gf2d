#include <SDL.h>
#include "simple_logger.h"
#include "gfc_config.h"
#include "gf2d_draw.h"
#include "world.h"
#include "collisions.h"
#include "player_move.h"
#include "player_attacks.h"
#include "player.h"

// required entity functions
void player_think(Entity* self);
void player_update(Entity* self);
void player_gravity(Entity* self);
void player_free(Entity* self);

/**
* @brief initialize PlayerData
* @param data: json data to initialize PlayerData
*/
PlayerData* player_data_init(Entity* self, SJson* data);

static Entity* player;

Entity* player_spawn(GFC_Vector2D position, SJson* data) {
	SJson* curr_entry;

	player = entity_new();
	if (!player) {
		slog("failed to initialize player entity");
		return NULL;
	}

	player->type = PLAYER;
	gfc_line_cpy(player->name, "Player");	// change later

	curr_entry = sj_object_get_value(data, "entity_data");

	gfc_vector2d_copy(player->position, position);
	player->velocity = gfc_vector2d(0, 0);
	sj_object_get_vector2d(curr_entry, "max_velocity", &player->max_velocity);
	sj_object_get_vector2d(curr_entry, "accel", &player->accel);
	player->sprite = gf2d_sprite_load_image(sj_object_get_string(curr_entry, "sprite"));
	player->frame = 0;

	player->think = player_think;
	player->update = player_update;
	player->grav = player_gravity;
	player->grav_flag = 1;

	player->scale = gfc_vector2d(1, 1);
	player->dir = gfc_vector2d(0, 0);
	player->free = player_free;

	update_hurtbox(player);
	update_boundbox(player);

	player->data = player_data_init(player, sj_object_get_value(data, "player_data"));
	if (!player->data) {
		slog("failed to initialize player data");
		return NULL;
	}

	return player;
}

void player_free(Entity* self) {
	gf2d_sprite_delete(self->sprite);
	
	if (self->data) free(self->data);
}

PlayerData* player_data_init(Entity* self, SJson* data) {
	PlayerData* p_data;

	p_data = gfc_allocate_array(sizeof(PlayerData), 1);
	if (!p_data) return NULL;

	gfc_vector2d_copy(p_data->spawn_pos, self->position);
	p_data->state = IDLE;
	p_data->moveTypeX = NONE_X;
	p_data->moveTypeY = NONE_Y;
	p_data->jump_count = 0;

	sj_object_get_uint8(data, "max_jumps", &p_data->max_jumps);
	sj_object_get_int(data, "max_dodge_charges", &p_data->max_dodge_charges);
	p_data->dodge_charges = p_data->max_dodge_charges;
	sj_object_get_vector2d(data, "dodge_vel", &p_data->dodge_vel);
	sj_object_get_float(data, "dodge_vel_reduc", &p_data->dodge_vel_reduc);
	sj_object_get_float(data, "jump_speed", &p_data->jump_speed);
	sj_object_get_vector3d(data, "friction", &p_data->friction);

	return p_data;
}

void player_think(Entity* self) {
	PlayerData* p_data;

	p_data = self->data;

	player_move(self);
	
}

void player_update(Entity* self) {
	PlayerData* p_data;
	//GFC_Edge2D bottom;
	//float ground_level;

	p_data = self->data;
	if (!p_data) return;

	//ground_level = get_ground_level();
	//bottom = get_bottom_edge(self->boundbox.s.r);

	switch (p_data->moveTypeX) {
		case LEFT:
			self->dir.x = 1;
			break;
		case RIGHT:
			self->dir.x = 0;
			break;
	}

	if (self->position.y > RES.y + RES.h)
		gfc_vector2d_copy(self->position, p_data->spawn_pos);

	//slog("%i", wall_collision(self, 1));

	/*DEBUG: center checking*/
	gf2d_draw_rect(self->boundbox.s.r, GFC_COLOR_RED);

	/*
	gf2d_draw_line(
		gfc_vector2d(bottom.x1, bottom.y1),
		gfc_vector2d(bottom.x2, bottom.y2),
		GFC_COLOR_RED
	);*/
	
	/*
	gf2d_draw_line(
		gfc_vector2d(0, self->position.y),
		gfc_vector2d(1200, self->position.y),
		GFC_COLOR_BLUE
	);
	gf2d_draw_line(
		gfc_vector2d(self->position.x, 0),
		gfc_vector2d(self->position.x, 720),
		GFC_COLOR_BLUE
	);
	*/
}

void player_gravity(Entity* self) {
	PlayerData* p_data;
	GFC_Edge2D bottom;
	Uint8 i;

	p_data = self->data;
	if (!p_data) return;
	if (p_data->state == DODGE) return;

	bottom = get_edge_from_rect(self->boundbox.s.r, 0);

	i = ground_collision(self);
	if (i && !p_data->jump_count) { // grounded
		if (self->velocity.x == 0)
			p_data->turnaround = 0;

		if (self->velocity.x > 0)
			p_data->state = SLOWDOWN;
	}
	else if (i && p_data->jump_count) { // landing
		// reset some flags and values as needed before landing
		p_data->dodge_charges = p_data->max_dodge_charges;
		p_data->jump_count = 0;
		self->velocity.y = 0;
		p_data->wall_jump = 0;
	}
	else { // falling
		self->position.y -= self->velocity.y;

		if (ceiling_collision(self))
			self->velocity.y = 0; 

		// increase falling speed
		self->velocity.y -= p_data->moveTypeY == FASTFALLING ? self->accel.y : GRAVITY;

		// limit vertical velocity
		if (self->velocity.y < -self->max_velocity.y)
			self->velocity.y = -self->max_velocity.y;

		// change moveTypeY to FALLING once peak of jump has reached
		if (self->velocity.y <= 0.0f && self->velocity.y > -2.0f)
			p_data->moveTypeY = FALLING;

		// reset wall jump
		if (self->velocity.y <= 7.0f) 
			p_data->wall_jump = 0;

		// prevent MultiVersus DJ
		if (!p_data->jump_count)
			p_data->jump_count++;
	}
}

GFC_Vector2D* get_player_pos() {
	return &player->position;
}
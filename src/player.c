#include <SDL.h>
#include "simple_logger.h"
#include "gfc_config.h"
#include "gf2d_draw.h"
#include "gfc_input.h"
#include "world.h"
#include "level.h"
#include "player.h"

void player_think(Entity* self);
void player_update(Entity* self);
void player_gravity(Entity* self);
void player_free(Entity* self);

void player_move(Entity* self);
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
	p_data->moveType = NONE;
	p_data->jump_count = 0;

	sj_object_get_uint8(data, "max_jumps", &p_data->max_jumps);
	sj_object_get_int(data, "max_dodge_charges", &p_data->max_dodge_charges);
	p_data->dodge_charges = p_data->max_dodge_charges;
	sj_object_get_vector2d(data, "dodge_vel", &p_data->dodge_vel);

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

	switch (p_data->moveType) {
		case LEFT:
			self->dir.x = 1;
			break;
		case RIGHT:
			self->dir.x = 0;
			break;
	}

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

void player_move(Entity* self) {
	PlayerData* p_data;
	Uint8 i, j;

	p_data = self->data;
	if (!p_data) return;

	/*
	switch (p_data->state) {
		case MOVING:
			slog("moving");
			break;

		case SLOWDOWN:
			slog("slowdown");
			break;
	}
	*/

	/* BASE MOVEMENT */
	// to help maintain momentum in the air
	if (!gfc_input_command_pressed("moveright") && !gfc_input_command_pressed("moveleft")
		&& self->velocity.x > 0 && !ground_collision(self) && p_data->state != DODGE)
		p_data->state = MOVING;

	if ((gfc_input_command_released("moveleft") || gfc_input_command_released("moveright"))
		&& self->velocity.x > 0 && p_data->state != DODGE)
		p_data->state = SLOWDOWN;

	// input checks
	if ((gfc_input_command_down("moveleft") || gfc_input_command_pressed("moveleft"))
		&& !gfc_input_command_pressed("moveright") && p_data->state != DODGE) {
		p_data->state = MOVING;

		if (p_data->moveType == RIGHT && self->velocity.x > 0) {
			p_data->state = SLOWDOWN;
			if (ground_collision(self) == 2)
				p_data->turnaround = 1;
		}
		else
			p_data->moveType = LEFT;

		if (self->velocity.x <= self->max_velocity.x) // ground
			self->velocity.x += self->accel.x;
		else if (p_data->turnaround && self->velocity.x <= self->max_velocity.x) // air
			self->velocity.x += self->accel.y;
	}
	else if ((gfc_input_command_down("moveright") || gfc_input_command_pressed("moveright"))
		&& !gfc_input_command_pressed("moveleft") && p_data->state != DODGE) {
		p_data->state = MOVING;

		if (p_data->moveType == LEFT && self->velocity.x > 0) {
			p_data->state = SLOWDOWN;
			if (ground_collision(self) == 2)
				p_data->turnaround = 1;
		}
		else
			p_data->moveType = RIGHT;

		if (self->velocity.x <= self->max_velocity.x) // ground
			self->velocity.x += self->accel.x;
		else if (p_data->turnaround && self->velocity.x <= self->max_velocity.x) // air
			self->velocity.x += self->accel.y;
	}

	/* DODGE */
	if ((gfc_input_command_pressed("dodge")) && p_data->state != DODGE && p_data->dodge_charges > 0) {
		if (gfc_input_command_pressed("moveright") || self->dir.x == 0)
			p_data->moveType = RIGHT;
		else if (gfc_input_command_pressed("moveleft") || self->dir.x == 1)
			p_data->moveType = LEFT;

		p_data->state = DODGE;

		self->velocity.x = p_data->dodge_vel.x;
		if (!ground_collision(self)) {
			p_data->dodge_charges--;
			self->velocity.x = p_data->dodge_vel.y;
		}
		self->velocity.y = 0;
	}

	/* JUMP */
	// NOTE: positive vertical movement is negativ
	if (gfc_input_command_pressed("jump") && p_data->jump_count < p_data->max_jumps) {
		// go up
		self->velocity.y = self->max_velocity.y;
		p_data->jump_count++;
	}


	// big-ass state check to actually apply the movement
	i = self->dir.x == 0 ? 0 : 1;	// 0 == left wall, 1 == right wall
	if (p_data->state == MOVING) { // regular movement
		if (wall_collision(self, i) || entity_keep_in_bounds(self, i)) {
			self->velocity.x = 0;
			return;
		}

		if (self->velocity.x > self->max_velocity.x)
			self->velocity.x -= 0.1f;

		self->position.x += p_data->moveType == RIGHT ?
							self->velocity.x : -self->velocity.x;
	}
	else if (p_data->state == SLOWDOWN) {
		if (wall_collision(self, i) || entity_keep_in_bounds(self, i)) {
			self->velocity.x = 0;
			return;
		}

		// sliding effect
		self->position.x += p_data->moveType == RIGHT ?
							self->velocity.x : -self->velocity.x;

		// if on ground, apply friction
		if (ground_collision(self) == 2 && self->velocity.x > 0) {
			self->velocity.x -= p_data->turnaround ? 0.5f : 0.17f;

			if (self->velocity.x < 0) {
				self->velocity.x = 0;
				p_data->moveType = NONE;
				p_data->state = IDLE;
			}
		}
		else { // if in air and turning around, slow down
			self->velocity.x -= 0.7f;
			if (self->velocity.x < 0) {
				p_data->moveType = p_data->moveType != RIGHT ? RIGHT : LEFT;
				p_data->turnaround = 1;
				self->velocity.x = 0;
			}
		}
	}
	else if (p_data->state == DODGE) {
		if (entity_keep_in_bounds(self, i) || wall_collision(self, i)) {
			self->velocity.x = 0;
			return;
		}

		self->position.x += p_data->moveType == RIGHT ?
							self->velocity.x : -self->velocity.x;

		self->velocity.x -= 0.5f;

		// dodge jump momentum carrying
		if (!ground_collision(self) && p_data->jump_count > 1)
			p_data->state = MOVING;
		else if (!p_data->jump_count && ground_collision(self) == 2)
			p_data->state = SLOWDOWN;
		else if (self->velocity.x < 6.0f) {
			self->velocity.x = 6.0f;
			p_data->state = SLOWDOWN;
		}
	}
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
	if (i == 2 && !p_data->jump_count) { // grounded
		if (self->velocity.x == 0)
			p_data->turnaround = 0;

		if (self->velocity.x > 0)
			p_data->state = SLOWDOWN;
	}
	else if (i == 1) { // landing
		p_data->dodge_charges = p_data->max_dodge_charges;
		p_data->jump_count = 0;
		self->velocity.y = 0;
	}
	else { // falling
		self->position.y -= self->velocity.y;
		self->velocity.y -= GRAVITY;
	}
}

GFC_Vector2D* get_player_pos() {
	return &player->position;
}
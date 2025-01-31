#include <SDL.h>
#include "simple_logger.h"
#include "gf2d_draw.h"
#include "gfc_input.h"
#include "world.h"
#include "level.h"
#include "player.h"

void player_move(Entity* self);
void player_free(Entity* self);
PlayerData* player_data_init(Entity* self);
void player_gravity(Entity* self);

Entity* player_spawn(GFC_Vector2D position) {
	Entity* player;
	PlayerData* p_data;

	player = entity_new();
	if (!player) return NULL;

	gfc_line_cpy(player->name, "Player");	// change later

	gfc_vector2d_copy(player->position, position);
	player->velocity = gfc_vector2d(0, 0);
	player->max_velocity = gfc_vector2d(9.0f, 17.0f);

	player->accel = gfc_vector2d(0.08f, 0.2f);

	player->sprite = gf2d_sprite_load_image("images/test.png");
	player->think = player_think;
	player->update = player_update;
	player->scale = gfc_vector2d(1, 1);
	player->dir = gfc_vector2d(0, 0);
	player->free = player_free;
	update_hurtbox(player);
	update_boundbox(player);
	//player->bounds.s.r = gfc_rect(0, 0, 1200, 700);


	player->data = player_data_init(player);
	p_data = player->data;

	return player;
}

PlayerData* player_data_init(Entity* self) {
	PlayerData* p_data;

	p_data = gfc_allocate_array(sizeof(PlayerData), 1);
	if (!p_data) return NULL;

	gfc_vector2d_copy(p_data->spawn_pos, self->position);

	p_data->state = IDLE;
	p_data->moveType = NONE;

	return p_data;
}

void player_think(Entity* self) {
	PlayerData* p_data;

	p_data = self->data;

	player_move(self);
	
}

void player_update(Entity* self) {
	PlayerData* p_data;
	GFC_Edge2D bottom;

	p_data = self->data;
	if (!p_data) return;

	bottom = get_bottom_edge(self->boundbox.s.r);

	switch (p_data->moveType) {
		case LEFT:
			self->dir.x = 1;
			break;
		case RIGHT:
			self->dir.x = 0;
			break;
	}

	update_hurtbox(self);
	update_boundbox(self);

	player_gravity(self);

	/*DEBUG: center checking*/
	//gf2d_draw_rect(self->boundbox.s.r, GFC_COLOR_RED);

	gf2d_draw_line(
		gfc_vector2d(bottom.x1, bottom.y1),
		gfc_vector2d(bottom.x2, bottom.y2),
		GFC_COLOR_RED
	);

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
}

void player_move(Entity* self) {
	PlayerData* p_data;
	const Uint8* keys;
	keys = SDL_GetKeyboardState(NULL);

	p_data = self->data;
	if (!p_data) return;

	/* BASE MOVEMENT */
	if ((gfc_input_command_released("moveleft") || gfc_input_command_released("moveright")) && self->velocity.x > 0) {
			p_data->state = SLOWDOWN;
	}

	/*NOTE: positive vertical movement is negative*/
	if (gfc_input_command_down("moveleft") && !gfc_input_command_pressed("moveright")) {
		p_data->state = MOVING;
		p_data->moveType = LEFT;

		self->position.x -= self->velocity.x;

		if (self->velocity.x <= self->max_velocity.x && !p_data->jump_flag)
			self->velocity.x += self->accel.x;
	}
	if (gfc_input_command_down("moveright") && !gfc_input_command_pressed("moveleft")) {
		p_data->state = MOVING;
		p_data->moveType = RIGHT;

		self->position.x += self->velocity.x;

		if (self->velocity.x <= self->max_velocity.x && !p_data->jump_flag)
			self->velocity.x += self->accel.x;
	}

	// sliding effect
	if (p_data->state == SLOWDOWN) {
		if (p_data->jump_flag){
			if (p_data->moveType == RIGHT)
				self->position.x += self->velocity.x;
			else if (p_data->moveType == LEFT)
				self->position.x -= self->velocity.x;
		}
		else {
			if (p_data->moveType == RIGHT)
				self->position.x += self->velocity.x;
			else if (p_data->moveType == LEFT)
				self->position.x -= self->velocity.x;

			self->velocity.x -= 0.17f;
			if (self->velocity.x < 0) {
				self->velocity.x = 0;
				p_data->moveType = NONE;
				p_data->state = IDLE;
			}
		}
	}

	/* JUMP */
	if (keys[SDL_SCANCODE_SPACE] && !p_data->jump_flag) {
		// go up
		//slog("jump");
		self->velocity.y = self->max_velocity.y;
		p_data->jump_flag = 1;
	}

	/*
	if (p_data->jump_flag) {
		if (ground_collision(self)) {
			self->position.y = 349.0f;
			self->velocity.y = 0;
			p_data->jump_flag = 0;
		}
		else {
			self->position.y -= self->velocity.y;
			self->velocity.y -= GRAVITY;
		}
	}
	*/
}

void player_gravity(Entity* self) {
	PlayerData* p_data;
	GFC_Edge2D bottom;
	float ground_level;

	p_data = self->data;
	if (!p_data) return;

	ground_level = get_ground_level();
	bottom = get_bottom_edge(self->boundbox.s.r);

	if (ground_collision(self) && !p_data->jump_flag) { // grounded
		self->velocity.y = 0;
		if (p_data->jump_flag)
			p_data->jump_flag = 0;

		self->position.y = ground_level- self->sprite->frame_w / 2.0f + 1.0f;
	}
	else { // falling
		//slog("%f : %f", bottom.y1, ground_level);

		self->position.y -= self->velocity.y;
		self->velocity.y -= GRAVITY;

		if (ground_collision(self) && p_data->jump_flag)
			p_data->jump_flag = 0;
	}
		
}

void player_free(Entity* self) {
	PlayerData* p_data;

	p_data = (PlayerData*) self->data;
	if (!p_data) return;

	gf2d_sprite_delete(self->sprite);
	free(p_data);
}
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

Entity* player_spawn(GFC_Vector2D position) {
	Entity* player;
	PlayerData* p_data;

	player = entity_new();
	if (!player) return NULL;

	gfc_line_cpy(player->name, "Player");	// change later

	gfc_vector2d_copy(player->position, position);
	player->velocity = gfc_vector2d(0, 0);
	player->max_velocity = gfc_vector2d(9.0f, 17.0f);
	player->velocity.y = player->max_velocity.y;

	player->accel = gfc_vector2d(0.08f, 0.2f);

	player->sprite = gf2d_sprite_load_image("images/test.png");
	player->think = player_think;
	player->update = player_update;
	player->scale = gfc_vector2d(1, 1);
	player->free = player_free;
	player->bounds.s.r = gfc_rect(0, 0, 1200, 700);

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

	p_data = self->data;
	
	update_hurtbox(self);
	gf2d_draw_rect(self->hurtbox.s.r, GFC_COLOR_RED);
}

void player_move(Entity* self) {
	PlayerData* p_data;
	const Uint8* keys;
	keys = SDL_GetKeyboardState(NULL);

	p_data = self->data;

	/* BASE MOVEMENT*/
	if (gfc_input_command_released("moveleft") || gfc_input_command_released("moveright")) {
		if (self->velocity.x > 0)
			p_data->state = SLOWDOWN;
	}

	/*NOTE: positive vertical movement is negative*/
	if (gfc_input_command_down("moveleft") && !gfc_input_command_pressed("moveright")) {
		p_data->state = MOVING;
		p_data->moveType = LEFT;

		self->position.x -= self->velocity.x;

		if (self->velocity.x <= self->max_velocity.x)
			self->velocity.x += self->accel.x;
	}
	if (gfc_input_command_down("moveright") && !gfc_input_command_pressed("moveleft")) {
		p_data->state = MOVING;
		p_data->moveType = RIGHT;

		self->position.x += self->velocity.x;

		if (self->velocity.x <= self->max_velocity.x)
			self->velocity.x += self->accel.x;
	}

	// sliding effect
	if (p_data->state == SLOWDOWN) {
		if (p_data->moveType == RIGHT)
			self->position.x += self->velocity.x / 5.0f;
		else if (p_data->moveType == LEFT)
			self->position.x -= self->velocity.x / 5.0f;

		self->velocity.x -= 0.25f;
		if (self->velocity.x < 0) {
			self->velocity.x = 0;
			p_data->moveType = NONE;
			p_data->state = IDLE;
		}
	}

	/* JUMP*/
	if (keys[SDL_SCANCODE_SPACE] && !p_data->jump_flag) {
		// go up
		self->velocity.y = self->max_velocity.y;
		p_data->jump_flag = 1;
	}

	if (p_data->jump_flag) {
		self->position.y -= self->velocity.y;

		self->velocity.y -= GRAVITY;
		if (ground_collision(self)) {
			self->position.y = 349.0f;
			self->velocity.y = 0;
			p_data->jump_flag = 0;
		}

	}
}

void player_free(Entity* self) {
	PlayerData* p_data;

	p_data = (PlayerData*) self->data;
	if (!p_data) return;

	gf2d_sprite_delete(self->sprite);
	free(p_data);
}
#include <SDL.h>
#include "simple_logger.h"
#include "gfc_input.h"
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
	player->sprite = gf2d_sprite_load_image("images/test.png");
	player->think = player_think;
	player->update = player_update;
	player->scale = gfc_vector2d(0.25, 0.25);
	player->free = player_free;
	player->bounds.s.r = gfc_rect(0, 0, 1200, 720);

	player->data = player_data_init(player);
	p_data = player->data;



	return player;
}

PlayerData* player_data_init(Entity* self) {
	PlayerData* p_data;

	p_data = gfc_allocate_array(sizeof(PlayerData), 1);
	if (!p_data) return NULL;

	gfc_vector2d_copy(p_data->spawn_pos, self->position);

	p_data->velocity = gfc_vector2d(0, 0);
	p_data->max_velocity = gfc_vector2d(9.0f, 9.0f);

	p_data->accel = gfc_vector2d(0.08f, 0.2f);

	p_data->state = IDLE;
	p_data->moveType = NONE;

	return p_data;
}

void player_think(Entity* self) {

	
}

void player_update(Entity* self) {
	PlayerData* p_data;

	p_data = self->data;
	
	player_move(self);
}

void player_move(Entity* self) {
	PlayerData* p_data;

	p_data = self->data;

	if (gfc_input_command_released("moveleft") || gfc_input_command_released("moveright"))
		p_data->state = IDLE;

	/*positive vertical movement is negative*/
	if (gfc_input_command_down("moveleft") && !gfc_input_command_pressed("moveright")) {
		p_data->state = MOVING;
		p_data->moveType = LEFT;

		self->position.x -= p_data->velocity.x;

		if (p_data->velocity.x <= p_data->max_velocity.x)
			p_data->velocity.x += p_data->accel.x;
	}	
	if (gfc_input_command_down("moveright") && !gfc_input_command_pressed("moveleft")) {
		p_data->state = MOVING;
		p_data->moveType = RIGHT;

		self->position.x += p_data->velocity.x;

		if (p_data->velocity.x <= p_data->max_velocity.x)
			p_data->velocity.x += p_data->accel.x;
	}

	// sliding effect
	if (p_data->state != MOVING && p_data->velocity.x > 0) {
		if (p_data->moveType == RIGHT)
			self->position.x += p_data->velocity.x / 5.0f;
		else if (p_data->moveType == LEFT)
			self->position.x -= p_data->velocity.x / 5.0f;

		p_data->velocity.x -= 0.25f;
		if (p_data->velocity.x < 0) {
			p_data->velocity.x = 0;
			p_data->moveType = NONE;
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
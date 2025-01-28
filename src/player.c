#include <SDL.h>
#include "simple_logger.h"
#include "player.h"

void player_move(Entity* self);
void player_free(Entity* self);

Entity* player_spawn(GFC_Vector2D position) {
	Entity* player;
	PlayerData* p_data;

	player = entity_new();
	if (!player) return NULL;

	p_data = gfc_allocate_array(sizeof(PlayerData), 1);
	if (p_data)
		player->data = p_data;
	else
		return NULL;

	gfc_line_cpy(player->name, "Player");	// change later

	slog("here");

	gfc_vector2d_copy(player->position, position);
	player->sprite = gf2d_sprite_load_image("images/test.png");
	player->think = player_think;
	player->update = player_update;
	player->scale = gfc_vector2d(0.25, 0.25);
	player->free = player_free;
	player->bounds.s.r = gfc_rect(0, 0, 1200, 720);

	gfc_vector2d_copy(p_data->spawn_pos, position);

	return player;
}

void player_think(Entity* self) {

	
}

void player_update(Entity* self) {
	player_move(self);
}

void player_move(Entity* self) {
	const Uint8* keys;
	keys = SDL_GetKeyboardState(NULL);

	/*positive vertical movement is negative*/
	if (keys[SDL_SCANCODE_A]) {
		self->position.x -= 5.0f;
	}	
	if (keys[SDL_SCANCODE_D]) {
		self->position.x += 5.0f;
	}
	if (keys[SDL_SCANCODE_W]) {
		self->position.y -= 5.0f;
	}
	if (keys[SDL_SCANCODE_S]) {
		self->position.y += 5.0f;
	}

}

void player_free(Entity* self) {
	PlayerData* p_data;

	p_data = (PlayerData*) self->data;
	if (!p_data) return;

	gf2d_sprite_delete(self->sprite);
	free(p_data);
}
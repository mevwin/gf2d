#include "simple_logger.h"
#include "player.h"

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
	player->free = player_free;

	gfc_vector2d_copy(p_data->spawn_pos, position);

	return player;
}

void player_think(Entity* self) {

}

void player_update(Entity* self) {
	
}

void player_free(Entity* self) {
	PlayerData* p_data;

	p_data = (PlayerData*) self->data;
	if (!p_data) return;

	gf2d_sprite_delete(self->sprite);
	free(p_data);
}
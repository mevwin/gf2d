#include "simple_logger.h"
#include "world.h"
#include "collisions.h"
#include "item.h"

void item_think(Entity* self);
void item_update(Entity* self);
void item_gravity(Entity* self);
void item_free(Entity* self);

void item_activate(Entity* self, ItemType type);

Entity* item_spawn(const char* item_name, GFC_Vector2D position) {
	SJson* file, * item_list, * init_data;
	//GFC_List* item_list;
	GFC_TextBlock text;
	ItemData* i_data;
	Entity* item;
	int i;

	item = entity_new();
	if (!item) {
		slog("failed to initialize item");
		return;
	}
	item->type = ITEM;
	gfc_vector2d_copy(item->position, position);
	item->think = item_think;
	item->update = item_update;
	item->grav = item_gravity;
	item->free = item_free;

	i_data = gfc_allocate_array(sizeof(ItemData), 1);
	if (!i_data) {
		slog("couldn't allocate space for item");
		entity_free(item);
		free(i_data);
		return;
	}
	item->data = i_data;
	i_data->active = 0;

	file = sj_load("def/item.def");
	if (!file) {
		slog("couldn't get item.def");
		entity_free(item);
		free(i_data);
		return;
	}
	item_list = sj_object_get_value(file, "items");

	init_data = NULL;
	for (i = 0; i < item_list->v.array->count; i++) {
		init_data = sj_array_get_nth(item_list, i);
		if (!init_data) continue;

		gfc_word_cpy(text, sj_object_get_string(init_data, "name"));
		if (!gfc_word_cmp(text, item_name)) {
			gfc_word_cpy(item->name, item_name);
			break;
		}
	}
	if (!init_data) {
		slog("%s not found", item_name);
		entity_free(item);
		free(i_data);
		return;
	}

	sj_object_get_uint8(init_data, "grav_flag", &item->grav_flag);

	sj_object_get_int(init_data, "interact_type", &i);
	i_data->interactType = (ItemInteractType) i;
	
	sj_object_get_int(init_data, "type", &i);
	i_data->type = (ItemType) i;
	switch (i_data->type) {
		case ITEM_ABILITY_UNLOCK:
			gfc_word_cpy(i_data->effect_value.text, sj_object_get_string(init_data, "effect_value"));
			//slog("%s", i_data->effect_value.text);
			break;
		
		case ITEM_HEALTH_PICKUP:
			sj_object_get_int(init_data, "effect_value", &i_data->effect_value.num);
			//slog("%i", i_data->effect_value.num);
			break;
	}

	item->sprite = gf2d_sprite_load_image(sj_object_get_string(init_data, "sprite"));
	
	update_hurtbox(item);
	update_boundbox(item);

	return item;
}

Entity* item_dummy_spawn(SJson* data, ItemType i_type, GFC_Vector2D position) {
	ItemData* i_data;
	Entity* item;
	int i;

	if (!data) return;

	item = entity_new();
	if (!item) {
		slog("failed to initialize item");
		return;
	}
	item->type = ITEM;
	gfc_vector2d_copy(item->position, position);
	item->draw_flag = 0;

	item->think = item_think;
	item->update = item_update;
	item->grav = item_gravity;
	item->free = item_free;

	i_data = gfc_allocate_array(sizeof(ItemData), 1);
	if (!i_data) {
		slog("couldn't allocate space for item");
		entity_free(item);
		free(i_data);
		return;
	}
	item->data = i_data;
	i_data->active = 0;

	gfc_word_cpy(item->name, sj_object_get_string(data, "name"));
	sj_object_get_int(data, "type", &i);
	i_data->type = (ItemType) i;

	sj_object_get_int(data, "interact_type", &i);
	i_data->interactType = (ItemInteractType)i;

	sj_object_get_uint8(data, "grav_flag", &item->grav_flag);

	switch (i_data->type) {
		case ITEM_ABILITY_UNLOCK:
			gfc_word_cpy(i_data->effect_value.text, sj_object_get_string(data, "effect_value"));
			//slog("%s", i_data->effect_value.text);
			break;

		case ITEM_HEALTH_PICKUP:
			sj_object_get_int(data, "effect_value", &i_data->effect_value.num);
			gfc_word_cpy(i_data->effect_value.text, sj_object_get_string(data, "effect_value_name"));
			//slog("%i", i_data->effect_value.num);
			break;
	}

	item->sprite = gf2d_sprite_load_image(sj_object_get_string(data, "sprite"));

	update_hurtbox(item);
	update_boundbox(item);

	return item;
}

void item_think(Entity* self) {
	ItemData* i_data;
	Entity* player;

	if (!self) return;

	i_data = self->data;
	if (!i_data) return;

	player = get_player();
	if (!player) {
		slog("no player found");
		return;
	}

	// check interaction
	if (!i_data->active) {
		switch (i_data->interactType) {
			case ITEM_INTERACT_PICK_UP:
				if (gfc_input_command_pressed("interact") && gfc_rect_overlap(player->boundbox.s.r, self->boundbox.s.r)) {
					i_data->active = 1;
					item_activate(self, i_data->type);
				}

				break;

			case ITEM_INTERACT_ATTRACT:

				break;

			default: //ITEM_INTERACT_STATIONARY

				break;
		}

	}
}

void item_update(Entity* self) {
	ItemData* i_data;

	if (!self) return;

	i_data = self->data;
	if (!i_data) return;

	update_hurtbox(self);
	update_boundbox(self);
}

void item_gravity(Entity* self) {
	ItemData* i_data;

	if (!self) return;

	i_data = self->data;
	if (!i_data) return;

}

void item_free(Entity* self) {
	if (!self) return;

	gf2d_sprite_free(self->sprite);

	if (self->data) free(self->data);
	level_free_level_spawn(self->name);
}

void item_activate(Entity* self, ItemType type) {
	ItemData* i_data;
	PlayerData* p_data;

	if (self) {
		i_data = (ItemData*) self->data;
		p_data = (PlayerData*) get_player_data();

		// implement item effects here
		switch (type) {
			case ITEM_ABILITY_UNLOCK:
				if (!gfc_word_cmp(i_data->effect_value.text, "DOUBLEJUMP")) {
					slog("unlocked double jump");
					p_data->canDoubleJump = 1;
					//slog("%i", p_data->canDoubleJump);
				}
				else if (!gfc_word_cmp(i_data->effect_value.text, "WALLJUMP")) {
					slog("unlocked wall jump");
					p_data->canWallJump = 1;
				}
				else if (!gfc_word_cmp(i_data->effect_value.text, "DODGE")) {
					slog("unlocked dodge");
					p_data->canDodge = 1;
				}
				else if (!gfc_word_cmp(i_data->effect_value.text, "RECALL")) {
					slog("unlocked recall");
					player_recall_init();
					p_data->canRecall = 1;
				}
				else if (!gfc_word_cmp(i_data->effect_value.text, "BASH")) {
					slog("unlocked bash");
					player_bash_init();
					p_data->canBash = 1;
				}

				break;
			case ITEM_HEALTH_PICKUP:
				p_data->currHealth += i_data->effect_value.num;
				if (p_data->currHealth > p_data->maxHealth)
					p_data->currHealth = p_data->maxHealth;

				break;
		}
	}

	entity_free(self);
}
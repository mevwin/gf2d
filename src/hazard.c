#include "simple_logger.h"
#include "gf2d_draw.h"
#include "world.h"
#include "collisions.h"
#include "player.h"
#include "hazard.h"

void hazard_think(Entity* self);
void hazard_update(Entity* self);
void hazard_free(Entity* self);

EntityAtk* create_geyser_sprout(Entity* h, SJson* data);

Entity* hazard_spawn(HazardType h_type, GFC_Vector2D position, const char* name) {
	Entity* hazard;
	HazardData* h_data;
	SJson* def, *list, *entry;
	GFC_Vector2D vec;

	def = sj_load("def/hazard.def");
	if (!def) {
		slog("failed to load hazard.def");
		return NULL;
	}

	hazard = entity_new();
	h_data = gfc_allocate_array(sizeof(HazardData), 1);

	gfc_word_cpy(hazard->name, name);
	gfc_vector2d_copy(hazard->position, position);
	hazard->type = HAZARD;
	h_data->h_type = h_type;

	hazard->think = hazard_think;
	hazard->update = hazard_update;
	hazard->free = hazard_free;

	hazard->grav_flag = 0;
	hazard->plat_flag = 0;
	hazard->canBeDamaged = 0;
	hazard->canBeBashed = 0;

	list = sj_object_get_value(def, "hazard_list");
	entry = sj_array_nth(list, h_type);
	switch (h_type) {
		case HAZARD_GEYSER:
			hazard->sprite = NULL;

			sj_object_get_vector2d(entry, "hurtbox", &vec);
			hazard->hurtbox.s.r = gfc_rect(position.x, position.y, vec.x, vec.y);
			h_data->geyser_initial_height = vec.y;
			sj_object_get_float(entry, "max_height", &h_data->geyser_max_height);

			h_data->color = sj_object_get_color(entry, "color");

			sj_object_get_vector2d(entry, "velocity", &hazard->velocity);
			sj_object_get_float(entry, "dragSpeed", &h_data->dragSpeed);

			// create attack for geyser
			h_data->geyser_sprout = create_geyser_sprout(hazard, entry);
			h_data->then = CURRENT_TIME;
			h_data->frame = 0;

			break;

		case HAZARD_VORTEX:
			hazard->sprite = gf2d_sprite_load_image(sj_object_get_string(entry, "sprite"));

			sj_object_get_vector2d(entry, "boundbox", &vec);
			hazard->boundbox.s.r = gfc_rect(position.x, position.y, vec.x, vec.y);
			hazard->boundbox.s.r.x -= vec.x * 0.5f;
			hazard->boundbox.s.r.y -= vec.y * 0.5f;
			update_hurtbox(hazard);

			sj_object_get_float(entry, "dragSpeed", &h_data->dragSpeed);

			break;
	}

	hazard->data = h_data;
	sj_free(def);

	return hazard;
}

Entity* hazard_dummy_spawn(SJson* data, HazardType h_type, GFC_Vector2D position) {
	Entity* hazard;
	HazardData* h_data;
	GFC_Vector2D vec;

	hazard = entity_new();
	h_data = gfc_allocate_array(sizeof(HazardData), 1);

	gfc_word_cpy(hazard->name, sj_object_get_string(data, "#name"));
	gfc_vector2d_copy(hazard->position, position);
	hazard->type = HAZARD;
	h_data->h_type = h_type;

	hazard->think = hazard_think;
	hazard->update = hazard_update;
	hazard->free = hazard_free;

	hazard->grav_flag = 0;
	hazard->plat_flag = 0;
	hazard->canBeDamaged = 0;
	hazard->canBeBashed = 0;

	switch (h_type) {
		case HAZARD_GEYSER:
			hazard->sprite = NULL;

			sj_object_get_vector2d(data, "hurtbox", &vec);
			hazard->hurtbox.s.r = gfc_rect(position.x, position.y, vec.x, vec.y);
			h_data->geyser_initial_height = vec.y;
			sj_object_get_float(data, "max_height", &h_data->geyser_max_height);

			h_data->color = sj_object_get_color(data, "color");

			sj_object_get_vector2d(data, "velocity", &hazard->velocity);
			sj_object_get_float(data, "dragSpeed", &h_data->dragSpeed);

			// create attack for geyser
			h_data->geyser_sprout = create_geyser_sprout(hazard, data);
			h_data->then = CURRENT_TIME;
			h_data->frame = 0;

			break;

		case HAZARD_VORTEX:
			hazard->sprite = gf2d_sprite_load_image(sj_object_get_string(data, "sprite"));

			sj_object_get_vector2d(data, "boundbox", &vec);
			hazard->boundbox.s.r = gfc_rect(position.x, position.y, vec.x, vec.y);

			sj_object_get_float(data, "dragSpeed", &h_data->dragSpeed);

			break;
	}

	hazard->data = h_data;

	return hazard;
}

EntityAtk* create_geyser_sprout(Entity* h, SJson* data) {
	EntityAtk* atk;

	atk = gfc_allocate_array(sizeof(EntityAtk), 1);
	gfc_word_cpy(atk->name, "geyser_sprout");
	atk->atk_type = 0;
	atk->isProj = 0;
	atk->active = 0;
	gfc_rect_copy(atk->hitbox, h->hurtbox.s.r);
	sj_object_get_float(data, "damage", &atk->damage);

	sj_object_get_uint32(data, "startupFrames", &atk->startupFrames);

	sj_object_get_uint32(data, "activeFrames", &atk->activeFrames);
	atk->activeFrames += atk->startupFrames;

	sj_object_get_uint32(data, "recovFrames", &atk->recovFrames);
	atk->recovFrames += atk->activeFrames;

	return atk;
}

void hazard_think(Entity* self) {
	HazardData* h_data = self->data;
	EntityAtk* g_sprout;
	Uint32 ent_max = getEntityMax();
	Entity* ent_list = getEntityList(), *ent;
	GFC_Vector2D direction;
	GFC_Rect rect;
	PlayerData* p_data;

	for (int i = 0; i < ent_max; i++) {
		ent = &ent_list[i];
		if (!ent->_inuse || ent->type == HAZARD || ent->type == ITEM) continue;

		switch (h_data->h_type) {
			case HAZARD_GEYSER:
				// push entities up
				g_sprout = h_data->geyser_sprout;
				gf2d_draw_rect_filled(g_sprout->hitbox, h_data->color);

				// make dummy rect for checks
				gfc_rect_copy(rect, g_sprout->hitbox);
				rect.y = g_sprout->hitbox.y + g_sprout->hitbox.h;
				rect.h = -rect.h;

				if (gfc_point_in_rect(ent->position, rect)) {
					ent->grav_flag = 0;
					ent->position.y -= h_data->dragSpeed;
				}
				else ent->grav_flag = 1;

				break;

			case HAZARD_VORTEX:
				// drag in entities
				if (gfc_rect_overlap(self->boundbox.s.r, ent->hurtbox.s.r) && !gfc_rect_overlap(self->hurtbox.s.r, ent->hurtbox.s.r)) {
					gfc_vector2d_sub(direction, self->position, ent->position);
					gfc_vector2d_set_magnitude(&direction, h_data->dragSpeed);
					gfc_vector2d_add(ent->position, ent->position, direction);
					ent->grav_flag = 0;

					if (ent->type == PLAYER) {
						p_data = ent->data;
						p_data->dodge_charges = p_data->max_dodge_charges;
					}
				}
				else ent->grav_flag = 1;

				break;
		}
	}
}

void hazard_update(Entity* self) {
	HazardData* h_data = self->data;
	EntityAtk* g_sprout;
	float time;

	time = CURRENT_TIME;

	switch (h_data->h_type) {
		case HAZARD_GEYSER:
			// update frame timing for sprout atk and hitbox height
			g_sprout = h_data->geyser_sprout;

			if (checkFramePass(h_data->then)) {
				h_data->frame++;
				h_data->then = time;
			}

			if (g_sprout->active) {
				self->hurtbox.s.r.h -= self->velocity.y;
				g_sprout->hitbox.h = self->hurtbox.s.r.h;
				if (self->hurtbox.s.r.h <= h_data->geyser_max_height)
					self->hurtbox.s.r.h = h_data->geyser_max_height;
			}

			if (h_data->frame == g_sprout->startupFrames) {
				g_sprout->active = 1;
			}
			else if (h_data->frame == g_sprout->activeFrames) {
				g_sprout->active = 0;
			}
			else if (h_data->frame == g_sprout->recovFrames) {
				h_data->frame = 0;
				self->hurtbox.s.r.h = h_data->geyser_initial_height;
				g_sprout->hitbox.h = self->hurtbox.s.r.h;
			}



			break;

		case HAZARD_VORTEX:
			gf2d_draw_rect_filled(self->boundbox.s.r, gfc_color8(0, 20, 20, 40));

			break;
	}
}

void hazard_free(Entity* self) {
	HazardData* h_data = self->data;

	if (self->sprite) gf2d_sprite_free(self->sprite);
	if (h_data->geyser_sprout) {
		if (!is_room_changing())
			free(h_data->geyser_sprout);
		else {
			// reset geyser frames and timing
			h_data->geyser_sprout->active = 0;
			h_data->frame = 0;
			h_data->then = CURRENT_TIME;
			self->hurtbox.s.r.h = h_data->geyser_initial_height;
		}
	}

	free(self->data);
}

void update_geyser_hurtbox(Entity* self) {
	HazardData* h_data = self->data;
	EntityAtk* g_sprout = h_data->geyser_sprout;

	self->hurtbox.s.r.x = self->position.x;
	self->hurtbox.s.r.y = self->position.y;
	g_sprout->hitbox.x = self->position.x;
	g_sprout->hitbox.y = self->position.y;
}

void update_vortex_boundbox(Entity* self) {
	self->boundbox.s.r.x = self->position.x;
	self->boundbox.s.r.y = self->position.y;
}
#include "simple_logger.h"
#include "gf2d_draw.h"
#include "world.h"
#include "collisions.h"
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
	switch (h_type) {
		case HAZARD_BREAK_WALL_VERT:
			entry = sj_array_nth(list, 0);
			entity_free(hazard);
			free(h_data);
			return NULL;

			break;

		case HAZARD_BREAK_WALL_HORIZ:
			entry = sj_array_nth(list, 1);
			entity_free(hazard);
			free(h_data);
			return NULL;

			break;

		case HAZARD_VORTEX:
			entry = sj_array_nth(list, 2);

			hazard->sprite = gf2d_sprite_load_image(sj_object_get_string(entry, "sprite"));

			sj_object_get_vector2d(entry, "boundbox", &vec);
			hazard->boundbox.s.r = gfc_rect(position.x, position.y, vec.x, vec.y);

			sj_object_get_float(entry, "dragSpeed", &h_data->dragSpeed);

			break;

		case HAZARD_GEYSER:
			entry = sj_array_nth(list, 3);

			hazard->sprite = NULL;

			sj_object_get_vector2d(entry, "hurtbox", &vec);
			hazard->hurtbox.s.r = gfc_rect(position.x, position.y, vec.x, vec.y);
			h_data->geyser_initial_height = vec.y;

			h_data->color = sj_object_get_color(entry, "color");

			sj_object_get_vector2d(entry, "velocity", &hazard->velocity);
			sj_object_get_float(entry, "dragSpeed", &h_data->dragSpeed);

			// create attack for geyser
			h_data->geyser_sprout = create_geyser_sprout(hazard, entry);
			h_data->then = CURRENT_TIME;
			h_data->frame = 0;

			break;
	}

	hazard->data = h_data;
	sj_free(def);

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

	switch (h_data->h_type) {
		case HAZARD_BREAK_WALL_VERT:


			break;

		case HAZARD_BREAK_WALL_HORIZ:


			break;

		case HAZARD_VORTEX:
			// drag in entities

			break;

		case HAZARD_GEYSER:
			// push entities up


			break;
	}
}

void hazard_update(Entity* self) {
	HazardData* h_data = self->data;
	EntityAtk* g_sprout;
	float time;

	time = CURRENT_TIME;

	switch (h_data->h_type) {
		case HAZARD_BREAK_WALL_VERT:


			break;

		case HAZARD_BREAK_WALL_HORIZ:


			break;

		case HAZARD_VORTEX:
			// drag in entities

			break;

		case HAZARD_GEYSER:
			// update frame timing for sprout atk and hitbox height
			g_sprout = h_data->geyser_sprout;

			if (checkFramePass(h_data->then)) {
				h_data->frame++;
				h_data->then = time;
			}

			if (g_sprout->active)
				self->hurtbox.s.r.h -= self->velocity.y;

			if (h_data->frame == g_sprout->startupFrames) {
				g_sprout->active = 1;
			}
			else if (h_data->frame == g_sprout->activeFrames) {
				g_sprout->active = 0;
			}
			else if (h_data->frame == g_sprout->recovFrames) {
				h_data->frame = 0;
				self->hurtbox.s.r.h = h_data->geyser_initial_height;
			}

			gf2d_draw_rect_filled(self->hurtbox.s.r, h_data->color);

			break;
	}
}

void hazard_free(Entity* self) {
	HazardData* h_data = self->data;

	if (self->sprite) gf2d_sprite_free(self->sprite);
	if (h_data->geyser_sprout) free(h_data->geyser_sprout);

	free(self->data);
}
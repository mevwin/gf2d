#include "simple_logger.h"
#include "gfc_config.h"
#include "gf2d_draw.h"
#include "world.h"
#include "collisions.h"
#include "projectile.h"

void proj_think(Entity* self);
void proj_update(Entity* self);
void proj_free(Entity* self);
//void proj_gravity(Entity* self);

void proj_spawn(char* name, GFC_Vector2D dir, GFC_Vector2D position, EntityType owner) {
	Entity* proj;
	ProjData* pj_data;
	SJson* def, *proj_arr, *test, *proj_entry;
	float move_mag;
	int i;

	proj = entity_new();
	if (!proj) {
		slog("failed to make projectile");
		return;
	}

	pj_data = gfc_allocate_array(sizeof(ProjData), 1);
	if (!pj_data) {
		slog("failed to allocate space for proj data");
		entity_free(proj);
		return;
	}

	def = sj_load("def/projectile.def");
	if (!def) {
		slog("failed to open projectile def file");
		entity_free(proj);
		free(pj_data);

		return;
	}
	
	proj->type = PROJECTILE;

	// find entry
	proj_arr = sj_object_get_value(def, "projectiles");
	proj_entry = NULL;
	for (i = 0; i < proj_arr->v.array->count; i++) {
		test = sj_array_get_nth(proj_arr, i);
		if (!strcmp(name, sj_object_get_string(test, "name"))) {
			proj_entry = test;
			break;
		}
	}

	if (!proj_entry) {
		slog("entry not found: %s", name);
		entity_free(proj);
		free(pj_data);
		sj_free(def);
		return;
	}

	proj->sprite = gf2d_sprite_load_image(sj_object_get_string(proj_entry, "sprite"));
	pj_data->owner = owner;

	// initializing movement
	gfc_vector2d_copy(proj->position, position);

	gfc_vector2d_copy(proj->velocity, dir);
	sj_object_get_float(proj_entry, "move_mag", &move_mag);
	gfc_vector2d_set_magnitude(&proj->velocity, move_mag);
	
	sj_object_get_vector2d(proj_entry, "accel", &proj->accel);

	/* other data */
	sj_object_get_uint8(proj_entry, "pierce", &pj_data->pierce);

	sj_object_get_uint8(proj_entry, "passThroughWalls", &pj_data->passThroughWalls);

	// offset position for the hitbox
	sj_object_get_vector2d(proj_entry, "hitbox", &pj_data->hitbox_dimen);
	update_hurtbox(proj);
	update_boundbox(proj);

	sj_object_get_float(proj_entry, "damage", &pj_data->damage);

	pj_data->active = 1;

	proj->data = pj_data;
	proj->think = proj_think;
	proj->update = proj_update;
	proj->free = proj_free;

	sj_free(def);
}

void proj_think(Entity* self) {
	ProjData* pj_data;

	if (!self) return;

	pj_data = (ProjData*)self->data;
	if (!pj_data) return;

	// move the projectile
	gfc_vector2d_add(self->position, self->position, self->velocity);
	//slog("%f, %f", self->velocity.x, self->velocity.y);

	// check if it hit anything
	if (!gfc_rect_overlap(self->hurtbox.s.r, RES)) {
		slog("hit nothing");
		entity_free(self);
	}
	else {
		// hit something
	}
	
}

void proj_update(Entity* self) {
	ProjData* pj_data;

	if (!self) return;

	pj_data = (ProjData*) self->data;
	if (!pj_data) return;

	// draw hurtbox
	gf2d_draw_rect_filled(self->hurtbox.s.r, gfc_color8(0, 120, 120, 120));

	// update hurtbox
	update_hurtbox(self);
	update_boundbox(self);
}

void proj_free(Entity* self) {
	if (!self) return;

	gf2d_sprite_free(self->sprite);
	free(self->data);
}
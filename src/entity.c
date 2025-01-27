#include "simple_logger.h"
#include "gfc_matrix.h"
#include "entity.h"

typedef struct EntityManager_S {
	Entity*		entityList;
	Uint32		entityMax;
}EntityManager;

static EntityManager ent_manager = { 0 };

void entity_system_init(Uint32 maxEnts) {
    //sanity check
    if (ent_manager.entityList) {
        slog("entity manager already exists");
        return;
    }

    //another sanity check
    if (!maxEnts) {
        slog("cannot allocated 0 entities for the entity manager");
        return;
    }
    ent_manager.entityList = gfc_allocate_array(sizeof(Entity), maxEnts);
    if (!ent_manager.entityList) {
        slog("failed to allocate %i entities for the entity manager", maxEnts);
        return;
    }
    ent_manager.entityMax = maxEnts; // at this point, big ass entity list is made
}

void entity_system_close() {
    int i;

    for (i = 0; i < ent_manager.entityMax; i++) {
        if (!ent_manager.entityList[i]._inuse) continue;
        entity_free(&ent_manager.entityList[i]);
    }

    free(ent_manager.entityList);
    memset(&ent_manager, 0, sizeof(EntityManager));
}

void entity_draw(Entity* self) {
    GFC_Matrix3 matrix; // not constructors in C

    if (self->draw) self->draw(self);

    gf2d_sprite_draw(
        self->sprite,
        self->position,
        &self->scale,
        NULL,
        NULL,
        NULL,
        NULL, 
        0
    );
}

void entity_draw_all() {
    int i;
    for (i = 0; i < ent_manager.entityMax; i++) {
        if (!ent_manager.entityList[i]._inuse) continue; // skips ones not inuse
        entity_draw(&ent_manager.entityList[i]);
    }
}

void entity_think(Entity* self) {
    if (!self) return;
    if (self->think) self->think(self);
}

void entity_think_all() {
    int i;
    for (i = 0; i < ent_manager.entityMax; i++) {
        if (!ent_manager.entityList[i]._inuse) continue; // skips ones not inuse
        entity_think(&ent_manager.entityList[i]);
    }
}

void entity_update(Entity* self) {
    if (!self) return;
    if (self->update) self->update(self);
}

void entity_update_all() {
    int i;
    for (i = 0; i < ent_manager.entityMax; i++) {
        if (!ent_manager.entityList[i]._inuse) continue; // skips ones not inuse
        entity_update(&ent_manager.entityList[i]);
    }
}

Entity* entity_new(Entity* self) {
    int i;
    for (i = 0; i < ent_manager.entityMax; i++)
    {
        if (ent_manager.entityList[i]._inuse) continue; // skips ones inuse
        memset(&ent_manager.entityList[i], 0, sizeof(Entity)); // clear out in case anything was still there

        // any default values should be set
        ent_manager.entityList[i]._inuse = 1;
        ent_manager.entityList[i].scale = gfc_vector2d(1, 1); // scale of zero means entity doesn't exist

        return &ent_manager.entityList[i];
    }
    slog("no more entity slots");
    return NULL; // no more entity slots
}

void entity_free(Entity* self) {
    // check if pointer is null
    if (!self) return;

    if (self->free) self->free(self);

    // free up anything that may have been allocated FOR this
    memset(self, 0, sizeof(Entity));
}

void update_hurtbox(Entity* self) {

}
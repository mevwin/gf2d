#include "simple_logger.h"
#include "gfc_matrix.h"
#include "entity.h"

// cite this page for later

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
    GFC_Vector2D offset, position;

    if (self->draw) self->draw(self);

    offset = gfc_vector2d(self->sprite->frame_w, self->sprite->frame_h);
    offset.x /= 2.0f;
    offset.y /= 2.0f;

    gfc_vector2d_sub(position, self->position, offset);

    // TODO: change sprite offset
    gf2d_sprite_draw(
        self->sprite,
        position,
        &self->scale,
        NULL,
        NULL,
        &self->dir,
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
    GFC_Vector2D offset, position;

    offset = gfc_vector2d(self->sprite->frame_w, self->sprite->frame_h);
    offset.x /= 2.0f;
    offset.y /= 2.0f;

    gfc_vector2d_sub(position, self->position, offset);
    self->hurtbox.s.r = gfc_rect(position.x, 
                                 position.y,
                                 self->sprite->frame_w,
                                 self->sprite->frame_h
        );
}

void update_boundbox(Entity* self) {
    // TODO: change later
    gfc_rect_copy(self->boundbox.s.r, self->hurtbox.s.r);   
}

GFC_Edge2D get_bottom_edge(GFC_Rect box) {
    GFC_Edge2D edge;

    edge = gfc_edge_from_vectors(
        gfc_vector2d(box.x, box.y + box.h),
        gfc_vector2d(box.x + box.w, box.y + box.h) 
    );

    return edge;
}
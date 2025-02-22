#include "simple_logger.h"
#include "gf2d_draw.h"
#include "gfc_matrix.h"
#include "world.h"
#include "collisions.h"
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
        self->frame
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
    if (self->update) { 
        self->update(self); 
        update_hurtbox(self);
        update_boundbox(self);
    }
}

void entity_update_all() {
    int i;
    for (i = 0; i < ent_manager.entityMax; i++) {
        if (!ent_manager.entityList[i]._inuse) continue; // skips ones not inuse
        entity_update(&ent_manager.entityList[i]);
    }
}

void entity_apply_grav(Entity* self) {
    if (!self) return;
    if (self->grav && self->grav_flag) self->grav(self);
}

void entity_apply_grav_all() {
    int i;
    for (i = 0; i < ent_manager.entityMax; i++) {
        if (!ent_manager.entityList[i]._inuse || !ent_manager.entityList[i].grav_flag) continue;
        entity_apply_grav(&ent_manager.entityList[i]);
    }
}

Uint8 entity_keep_in_bounds(Entity* self, Uint8 edge_type) {
    GFC_Edge2D screen_edge, e_edge;
    //GFC_Vector2D p1, p2;
    float offset;

    if (!self) return;
    if (self->bounds) self->bounds(self);
    // if no unique bounds function, at least restrict entity to viewspace
    offset = 1.0f;

    screen_edge = get_edge_from_rect(RES, edge_type + 2);
    e_edge = get_edge_from_rect(self->boundbox.s.r, edge_type + 2);
    e_edge.x1 += !edge_type ? self->velocity.x : -self->velocity.x;
        
    //slog("p: %f, s: %f", e_edge.x1, screen_edge.x1);

    if ((edge_type && e_edge.x1 <= screen_edge.x1 + offset)||
        (!edge_type && e_edge.x1 >= screen_edge.x1 - offset))
        return 1;
    else 
        return 0;

    /*
    screen_edge = get_edge_from_rect(RES, 1);
    e_edge = get_edge_from_rect(self->boundbox.s.r, 1);
    if (e_edge.y1 - self->velocity.y <= screen_edge.y1 + offset)
        return 0;

    screen_edge = get_edge_from_rect(RES, 0);
    e_edge = get_edge_from_rect(self->boundbox.s.r, 0);
    if (e_edge.y1 + self->velocity.y >= screen_edge.y1 - offset)
        return 0;
    */
}

Entity* entity_new() {
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
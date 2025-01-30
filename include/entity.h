#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "gfc_shape.h"
#include "gf2d_sprite.h"

#define MAX_ENTITY 500

typedef enum EntityType_S{
    PLAYER,
    ENEMY,
    PLATFORM,
    ITEM,
    HAZARD
}EntityType;

typedef struct Entity_S{
    Uint8           _inuse;         // flag for memory management
    GFC_TextLine    name;           // name of entity
    GFC_Vector2D    position;       // where it is in space
    GFC_Vector2D	velocity;
    GFC_Vector2D	max_velocity;
    GFC_Vector2D	accel;

    GFC_Vector2D    rotation;       // how to rotate it
    GFC_Vector2D    scale;          // stretching
    GFC_Vector2D    dir;
    Sprite*         sprite;         // graphics
    Uint32          frame;

    //behavior
    void (*think)   (struct Entity_S *self);    // called every frame for the entity to decide things
    void (*update)  (struct Entity_S *self);    // called every frame for the entity to update its state

    void (*free)    (struct Entity_S *self);    // called when the entity is cleaned up
    void (*draw)    (struct Entity_S* self);    // for custom draw calls
    void*           data;                       // entity data

    GFC_Shape       hurtbox;
    GFC_Shape       bounds;
}Entity;

/**
 * @brief initialize the entity manager subsystem
 * @param maxEnts how many entities can exist at the same time
 */
void entity_system_init(Uint32 maxEnts);

/**
 * @brief close the entity subsystem when game is closed
 */
void entity_system_close();

/**
 * @brief draw all active entities
 */
void entity_draw_all();

/**
 * @brief let all active entities think
 */
void entity_think_all();

/**
 * @brief let all active entities update
 */
void entity_update_all();

/**
 * @brief allocated a blank entity for use
 * @return NULL on failure (out of memory) or a pointer to the initialized entity
 */
Entity *entity_new();

/**
 * @brief return the memory of a previously allocated entity back to the pool
 * @param self the entity to free
 */
void entity_free(Entity* self);

void update_hurtbox(Entity* self);

Uint8 within_bounds(Entity* self);

#endif
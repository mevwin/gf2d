#ifndef __PROJECTILE_H__
#define __PROJECTILE_H__

#include "entity.h"

/*
typedef enum ProjType_E {
    PROJECTILE
}ProjType;
*/

typedef struct ProjData_E{
    EntityType      owner;
    float           damage;
    //Uint32          ttl;  //maybe
    Uint8           active;
    Uint8           pierce;
    Uint8           passThroughWalls;
    GFC_Vector2D    hitbox_dimen;
}ProjData;

void proj_spawn(char* name, GFC_Vector2D dir, GFC_Vector2D position, EntityType owner);


#endif
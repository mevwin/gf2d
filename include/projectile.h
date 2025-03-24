#ifndef __PROJECTILE_H__
#define __PROJECTILE_H__

#include "entity.h"

/*
typedef enum ProjType_E {
    PROJECTILE
}ProjType;
*/

typedef struct ProjData_E{
    Uint32          ttl;                
    Uint8           active;
    Uint8           pierce;
    Uint8           passThroughWalls;
}ProjData;

void proj_spawn(GFC_Vector2D position);


#endif
#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "entity.h"

typedef struct PlayerData_S{
	GFC_Vector2D	spawn_pos;

	GFC_Vector2D	vHoriz;
	GFC_Vector2D	vVert;

	float			currHealth;
	float			maxHealth;

	//TODO: insert resource bar here
	
}PlayerData;

Entity* player_spawn(GFC_Vector2D position);
void player_think(Entity* self);
void player_update(Entity* self);

#endif
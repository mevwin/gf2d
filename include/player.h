#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "entity.h"

typedef enum PlayerState_E{
	IDLE,
	MOVING,
	SLOWDOWN
}PlayerState;

typedef enum PlayerMove_E {
	LEFT,
	RIGHT,
	UP,
	DOWN,
	NONE
}PlayerMove;


typedef struct PlayerData_S{
	GFC_Vector2D	spawn_pos;
	PlayerState		state;
	PlayerMove		moveType;

	float			currHealth;
	float			maxHealth;

	// movement flags
	Uint8			jump_flag;

	// TODO: insert resource bar here
	// maybe add no_move toggle
}PlayerData;

Entity* player_spawn(GFC_Vector2D position);
void player_state_change(PlayerState new);
void player_think(Entity* self);
void player_update(Entity* self);

#endif
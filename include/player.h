#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "entity.h"

typedef enum PlayerState_E{
	IDLE,
	MOVING,
	SLOWDOWN,
	DODGE,
	WALLJUMP
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
	Uint8			jump_count;
	Uint8			max_jumps;
	Uint8			turnaround;
	Uint8			wall_jump;
	int				dodge_charges;
	int				max_dodge_charges;
	GFC_Vector2D	dodge_vel;		// x = grounded, y = aerial


	// TODO: insert resource bar here
	// maybe add no_move toggle
}PlayerData;

Entity* player_spawn(GFC_Vector2D position, SJson* data);
GFC_Vector2D* get_player_pos();

#endif
#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "entity.h"

typedef enum PlayerState_E{
	IDLE,
	MOVING,
	SLOWDOWN,
	DODGE
}PlayerState;

typedef enum PlayerMoveX_E {
	LEFT,
	RIGHT,
	NONE_X
}PlayerMoveX;

typedef enum PlayerMoveY_E {
	RISING,
	FALLING,
	FASTFALLING,
	NONE_Y
}PlayerMoveY;

typedef struct PlayerData_S{
	PlayerState		state;

	// player stats
	float			currHealth;
	float			maxHealth;

	// movement flags/checks
	PlayerMoveX		moveTypeX;
	PlayerMoveY		moveTypeY;
	Uint8			max_jumps;
	Uint8			wall_jump;
	Uint8			turnaround;
	
	// movement counters
	Uint8			jump_count;
	Uint8			dodge_charges;
	Uint8			max_dodge_charges;

	// movement values
	GFC_Vector2D	dodge_vel;		// x = grounded, y = aerial
	float			dodge_vel_reduc;
	float			jump_speed;
	GFC_Vector3D	friction;

	GFC_Vector2D	spawn_pos;

	// TODO: insert resource bar here
	// maybe add no_move toggle
}PlayerData;

Entity* player_spawn(GFC_Vector2D position, SJson* data);
GFC_Vector2D* get_player_pos();

#endif
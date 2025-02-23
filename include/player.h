#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "entity.h"
#include "player_move.h"

typedef enum PlayerState_E{
	IDLE,
	MOVING,
	SLOWDOWN,
	DODGE
}PlayerState;

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
	GFC_Vector2D	dodge_vel;
	float			dodge_vel_reduc;
	float			jump_speed;
	GFC_Vector3D	friction;

	GFC_Vector2D	spawn_pos;

	// TODO: insert resource bar here
	// maybe add no_move toggle
}PlayerData;

/**
* @brief spawn a player entity
* @param position: point to spawn the player in screen space
* @param data: pointer to json data to initialize data from
* @note: SPAWN ONLY ONE PLAYER
*/
Entity* player_spawn(GFC_Vector2D position, SJson* data);

/**
* @brief pointer to player's position
* @note used for circumstances where player position is needed w/o needing the player entity
*/
GFC_Vector2D* get_player_pos();

#endif
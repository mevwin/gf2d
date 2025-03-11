#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "entity.h"
#include "player_move.h"

typedef enum PlayerState_E{
	PLAYER_IDLE,			// no movement
	PLAYER_MOVING,			// currently moving
	PLAYER_SLOWDOWN,		// slowing down on surface
	
	// special states
	PLAYER_DODGE,			// currently dodging
	PLAYER_RECALL,
	PLAYER_BASH,
	PLAYER_KNOCKBACK,			// TODO: fix later
	PLAYER_DEAD
}PlayerState;

typedef struct PlayerData_S{
	PlayerState		state;

	// player stats
	float			currHealth;
	float			maxHealth;
	float           dmg_reduction;

	// player ability/upgrade checks
	Uint8			canRecall;
	Uint8			canDoubleJump;
	Uint8			canBash;
	Uint8			canWallJump;
	Uint8			canDodge;

	// movement flags/checks
	PlayerMoveX		moveTypeX;
	PlayerMoveY		moveTypeY;
	Uint8			max_jumps;
	Uint8			wall_jump;
	Uint8			turnaround;
	Uint8			isAttacking;
	Uint8			canMove;
	
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

void player_respawn(Entity* self, PlayerData* p_data);

/**
* @brief pointer to player's position
* @note used for circumstances where player position is needed w/o needing the player entity
*/
GFC_Vector2D* get_player_pos();

#endif
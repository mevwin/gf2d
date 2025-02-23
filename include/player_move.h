#ifndef __PLAYER_MOVE_H
#define __PLAYER_MOVE_H

#include "gfc_input.h"

typedef enum PlayerMoveX_E {
	PMOVE_LEFT,
	PMOVE_RIGHT,
	PMOVE_NONE_X
}PlayerMoveX;

typedef enum PlayerMoveY_E {
	PMOVE_RISING,
	PMOVE_FALLING,
	PMOVE_FASTFALLING,
	PMOVE_NONE_Y
}PlayerMoveY;

void player_move_system_init();

/**
* @brief function for basic movement options based on inputs
* @note always being called in think function
*/
void player_move(void* p);

void track_player(void* p);

/**
* @brief recall ability: return player to a previous position while restoring resources
*/
void player_recall(void* p);

// TODO: fix later
void player_bash(void* p);

#endif
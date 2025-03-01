#ifndef __PLAYER_MOVE_H__
#define __PLAYER_MOVE_H__

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

typedef enum RecallState_E {
	RECALL_NONE,
	RECALL_START,
	RECALL_REWIND,
	RECALL_STOP,
	RECALL_FINISH
}RecallState;

typedef enum BashState_E {
	BASH_NONE,
	BASH_START,
	BASH_MOVE,
	BASH_STOP
}BashState;

typedef struct RecallPoint_S {
	GFC_Vector2D	point;
	float			time;
}RecallPoint;

/**
* @brief function for basic movement options based on inputs
* @note always being called in think function
* @note wall collisions are handled here
*/
void player_move(void* p);

/**
* @brief track previous player positions for recall ability
*/
void track_player(void* p, void* data);

void player_recall_init();

/**
* @brief recall ability: return player to a previous position while restoring resources
*/
void player_recall(void* p, void* data);

void player_recall_reset(void* p, void* data);

RecallPoint* create_recall_pos(GFC_Vector2D pos, float time);

// TODO: fix later
void player_bash(void* p);

#endif
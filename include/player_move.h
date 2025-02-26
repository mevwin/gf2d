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

typedef enum RecallState_E {
	RECALL_NONE,
	RECALL_START,
	RECALL_REWIND,
	RECALL_STOP
}RecallState;

typedef struct RecallPosition_S {
	GFC_Vector2D	point;
	float			time;
}RecallPosition;


/**
* @brief function for basic movement options based on inputs
* @note always being called in think function
*/
void player_move(void* p);

/**
* @brief track previous player positions for recall ability
*/
void track_player(void* p, void* data);

void player_recall_init();

RecallPosition* create_recall_pos(GFC_Vector2D pos, float time);

// TODO: fix later
void player_bash(void* p);

#endif
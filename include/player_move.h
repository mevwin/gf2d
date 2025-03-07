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
	BASH_WAIT,
	BASH_MOVE,
	BASH_COLLIDED_MOVE,
	BASH_STOP
}BashState;

typedef enum BashDir_E {
	BASH_LEFT,
	BASH_RIGHT,
	BASH_UP,
	BASH_DOWN
}BashDir;

/**
* @brief function for basic movement options based on inputs
* @note always being called in think function
* @note wall collisions for player are handled here
*/
void player_move(void* p);

/**
* @brief track previous player positions at defined intervals for recall ability
* @param p: point to player entity (must be casted to Entity* in definition)
* @param p: point to player entity's data (must be casted to PlayerData* in defintion)
*/
void track_player(void* p, void* data);


/**
* @brief initialize player recall manager
*/
void player_recall_init();

/**
* @brief initalize player bash manager
*/
void player_bash_init();

/**
* @brief recall ability: return player to a previous position while restoring resources
* @param p: point to player entity (must be casted to Entity* in definition)
* @param p: point to player entity's data (must be casted to PlayerData* in defintion)
*/
void player_recall(void* p, void* data);

/**
* @brief reset player values and recall manager values
* @param p: point to player entity (must be casted to Entity* in definition)
* @param p: point to player entity's data (must be casted to PlayerData* in defintion)
*/
void player_recall_reset(void* p, void* data, float time);

/**
* @brief bash ability: player dashes in a cardinal direction from a nearby entity (if a projectile, reverse its direction)
* @param p: point to player entity (must be casted to Entity* in definition)
* @param p: point to player entity's data (must be casted to PlayerData* in defintion)
*/
void player_bash(void* p, void* data);

#endif
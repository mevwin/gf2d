#ifndef __PLAYER_MOVE_H
#define __PLAYER_MOVE_H

#include "gfc_input.h"

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
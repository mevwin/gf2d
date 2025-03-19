#ifndef __WORLD_H__
#define __WORLD_H__

#include "gfc_types.h"
#include "gfc_config.h"
#include "gfc_list.h"

#define GRAVITY 0.3f
#define RES gfc_rect(0,0,1200, 720)
#define CURRENT_TIME (SDL_GetTicks() * 0.001f)
#define FRAME_DUR 0.016f

typedef enum WorldState_E {
	// UI states
	WORLD_MAINMENU,
	WORLD_PAUSEMENU,
	WORLD_INGAME,
	WORLD_LEVELCOMPLETE,
	WORLD_PLAYERDEAD,
	WORLD_GAME_COMPLETE,

	// in-between states
	WORLD_GAMESTART,	
	WORLD_RESTART_LEVEL,
	WORLD_LOAD_NEXT_LEVEL,
	WORLD_GAMECLOSE,
	WORLD_CLOSE
}WorldState;

void world_init();
void world_update();
Uint8 close_game_check();
void change_world_state(WorldState new_state);
GFC_List* get_enemy_list();

void* get_player();
void* get_player_data();
//GFC_List* get_item_list();
/**
* @brief pointer to player's position
* @note used for circumstances where player position is needed w/o needing the player entity
*/
GFC_Vector2D get_player_pos();


#endif
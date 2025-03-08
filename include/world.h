#ifndef __WORLD_H__
#define __WORLD_H__

#include "gfc_types.h"
#include "gfc_config.h"
#define GRAVITY 0.3f
#define RES gfc_rect(0,0,1200, 700)
#define CURRENT_TIME (SDL_GetTicks() * 0.001f)

typedef enum WorldState_E {
	WORLD_MAINMENU,
	WORLD_GAMESTART,
	WORLD_INGAME,
	WORLD_PAUSEMENU,
	WORLD_PLAYERDEAD,
	WORLD_LEVELCOMPLETE,
	WORLD_GAMECLOSE
}WorldState;

void world_init();
void world_update();
Uint8 close_game_check();
void change_world_state(WorldState new_state);
//void world_append_enemy(void* enemy);

//WorldState getWorldState();

#endif
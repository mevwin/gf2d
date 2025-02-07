#ifndef __WORLD_H__
#define __WORLD_H__

#include "gfc_types.h"
#include "gfc_config.h"
#define GRAVITY 0.3f
#define RES gfc_rect(0,0,1200, 700)

typedef enum WorldState_E {
	INGAME,
	MAIN_MENU,
	GAME_PAUSED
}WorldState;

void world_init();
void world_update();
void world_done_change();
Uint8 world_done_check();
void world_append_enemy(void* enemy);

#endif
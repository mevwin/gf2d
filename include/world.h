#ifndef __WORLD_H__
#define __WORLD_H__

#define GRAVITY 0.3f

#include "gfc_types.h"

typedef enum WorldState_E {
	INGAME,
	MENU
}WorldState;

void world_init();
void world_update();
void world_done_change();
Uint8 world_done_check();
void world_append_enemy(void* enemy);

#endif
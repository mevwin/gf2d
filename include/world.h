#ifndef __WORLD_H__
#define __WORLD_H__

typedef enum WorldState_E {
	INGAME,
	MENU
}WorldState;

void world_init();
void world_update();


#endif
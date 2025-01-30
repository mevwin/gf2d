#ifndef __WORLD_H__
#define __WORLD_H__

#define GRAVITY 0.4f

typedef enum WorldState_E {
	INGAME,
	MENU
}WorldState;

void world_init();
void world_update();


#endif
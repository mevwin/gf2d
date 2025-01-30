#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "gfc_shape.h"

typedef enum LevelType_E {
	REGULAR
}LevelType;

typedef struct Level_S {
	LevelType		level_type;
	GFC_Rect		ground;
	GFC_List*		enemy_list;
	GFC_List*		rooms;
	Uint8			room_num;
}Level;

void level_manager_init();
Level* level_load(Uint8 index);
void level_update();
void level_close(Level* level);
Uint8 ground_collision(void* ent);

#endif 

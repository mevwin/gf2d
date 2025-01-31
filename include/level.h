#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "gfc_shape.h"

typedef enum LevelType_E {
	REGULAR
}LevelType;

typedef struct Level_S {
	//LevelType		level_type;

	// level contents
	//GFC_List*		enemy_list;

	// idk yet
	//GFC_List*		rooms;
	//Uint8			room_num;

	// positioning
	GFC_Rect		ground;
	GFC_Vector2D	player_spawn;
	//GFC_List*		enemy_spawns;
	//GFC_List*		item_spawns;
}Level;

void level_manager_init();
Level* level_load(Uint8 index);
void level_update();
void level_close(Level* level);
Level* get_curr_level();
float get_ground_level();
Uint8 ground_collision(void* ent);

#endif 

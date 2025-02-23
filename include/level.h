#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "gfc_shape.h"

typedef enum LevelType_E {
	LEVEL_TYPE_REGULAR
}LevelType;

typedef enum WallType_E {
	WALL_LEFT,
	WALL_RIGHT
}WallType;

typedef struct Wall_S {
	GFC_Edge2D		dimensions;
	WallType		type; // left = 0, right = 1
	Uint8			wjumpable;
}Wall;

typedef struct Ground_S {
	GFC_Rect		dimensions;
	GFC_Vector2D	region;
	GFC_Color		color;
	Uint8			wall_flag; //has active walls
	Uint8			ceil_flag;
	//Sprite*			sprite;
}Ground;

typedef struct Platform_S {
	GFC_Rect		dimensions;
	GFC_Vector2D	region;
	Uint8			moving;
	//Sprite*			sprite;
	// TODO: add specifics later
}Platform;

typedef struct Level_S {
	//LevelType		level_type;

	// level contents
	//GFC_List*		enemy_list;

	// idk yet
	//GFC_List*		rooms;
	//Uint8			room_num;

	// positioning
	//Ground*			curr_ground;
	GFC_Vector2D	player_spawn;
	GFC_List*		ground_list;
	GFC_List*		platform_list;
	GFC_List*		wall_list;
	//GFC_List*		enemy_spawns;
	//GFC_List*		item_spawns;
}Level;



void level_manager_init(const char* filename);
Level* level_load(Uint8 index);

void level_update();
void level_close(Level* level);
Level* get_curr_level();
Uint8 entity_keep_in_bounds(void* e, Uint8 edge_type);

#endif 

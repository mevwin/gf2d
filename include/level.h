#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "gfc_shape.h"
#include "gf2d_sprite.h"

typedef enum LevelType_E {
	LEVEL_TYPE_REGULAR
}LvlType;

typedef enum LevelObjective_E {
	LEVEL_OBJ_KILL_ALL_ENEMIES,
	LEVEL_OBJ_SURVIVE,
	LEVEL_OBJ_COLLECT
}LvlObjective;

typedef struct LevelSpawn_S {
	GFC_Vector2D		position;
	EntityType			ent_type;
	union {
		GFC_TextBlock	item_name;
		Uint8			enemy_type;
		Uint8			hazard_type;
	}type;
}LevelSpawn;

typedef enum WallType_E {
	WALL_LEFT,
	WALL_RIGHT
}WallType;

typedef struct Wall_S {
	GFC_Edge2D			dimensions;
	WallType			type; // left = 0, right = 1
	Uint8				wjumpable;
}Wall;

typedef struct Ground_S {
	GFC_Rect			dimensions;
	GFC_Vector2D		region;
	GFC_Color			color; 		// remove later
	Uint8				ceil_flag;
	Uint8				wall_flag;  // has active walls
	Wall				walls[2];
	//Sprite*			sprite;
}Ground;

typedef enum PlatformMove_E {
	PLATFORM_MOVE_NONE,
	PLATFORM_MOVE_LEFT,
	PLATFORM_MOVE_RIGHT,
	PLATFORM_MOVE_UP,
	PLATFORM_MOVE_DOWN
}PlatformMove;

typedef struct Platform_S {
	GFC_Rect			dimensions;
	GFC_Vector2D		region;
	Uint8				pass_through;
	Uint8				moving;

	// if moving platform
	GFC_Vector2D		move_speed;
	GFC_Vector4D		move_bounds;
	PlatformMove		moveType;
	//Sprite*			sprite;
	// TODO: add specifics later
}Platform;

typedef struct Room_S {
	GFC_TextWord		name;
	GFC_Vector2D		player_spawn;
	
	Uint8				ground_count;
	Ground*				grounds;

	Uint8				platform_count;
	Platform*			platforms;

	GFC_List*			entity_spawns;
}Room;

typedef struct Level_S {
	GFC_TextWord	name;
	LvlType			level_type;
	LvlObjective	objective;
	//Uint32			goal;
	//Uint32			goal_counter;

	Uint8			start_room;
	Uint8			room_count;
	Room*			rooms;
}Level;

void level_manager_init(const char* filename);
void level_load(Uint8 index);

void load_current_room();
void load_next_level(void* p);
void restart_level(void* p);

void level_update();
//void level_camera_update(GFC_Vector2D move_speed);
void level_curr_close();

Level* get_curr_level();
Room* get_current_room();
Uint8 entity_keep_in_bounds(void* e, Uint8 edge_type);

#endif 
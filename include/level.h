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
}Level;

#endif 

#ifndef __ITEM_H__
#define __ITEM_H__

#include "entity.h"

typedef enum ItemType_E {
	ITEM_NONE,
	ITEM_ABILITY_UNLOCK,
	ITEM_HEALTH_PICKUP
}ItemType;

typedef enum ItemInteractType {
	ITEM_INTERACT_PICK_UP,
	ITEM_INTERACT_ATTRACT,
	ITEM_INTERACT_STATIONARY
}ItemInteractType;

typedef struct ItemData_S {
	ItemType			type;
	ItemInteractType	interactType;

	float				effect_value;
	Uint8				active;
}ItemData;

//IF AN ENEMY HOLDS AN ITEM, IT WILL BE CREATED AFTER THE ENEMY DIES

#endif
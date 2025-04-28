#ifndef __ITEM_H__
#define __ITEM_H__

#include "player.h"

typedef enum ItemType_E {
	ITEM_NONE,
	ITEM_ABILITY_UNLOCK,
	ITEM_HEALTH_PICKUP
}ItemType;

typedef enum ItemInteractType {
	ITEM_INTERACT_PICK_UP,		// must press button to pick up
	ITEM_INTERACT_ATTRACT,		// item goes to player if near it
	ITEM_INTERACT_STATIONARY	// player needs to go to the item to get it
}ItemInteractType;

typedef struct ItemData_S {
	ItemInteractType	interactType;
	ItemType			type;
	union {
		GFC_TextWord	text;
		int				num;
	}effect_value;
	Uint8				active;
}ItemData;

Entity* item_spawn(const char* item_name, GFC_Vector2D position);
Entity* item_dummy_spawn(SJson* data, ItemType i_type, GFC_Vector2D position);

//IF AN ENEMY HOLDS AN ITEM, IT WILL BE CREATED AFTER THE ENEMY DIES

#endif
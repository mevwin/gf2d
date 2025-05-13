#ifndef __HAZARD_H__
#define __HAZARD_H__

#include "entity.h"

typedef enum HazardType_E {
	HAZARD_GEYSER,
	HAZARD_VORTEX
}HazardType;

typedef struct HazardData_S {
	HazardType		h_type;

	float			dragSpeed;				// geyser/vortex: how fast to move other entities
	float			geyser_initial_height;	// geyser
	float			geyser_max_height;		// geyser
	GFC_Color		color;					// geyser

	// geyser attack
	float			then;
	Uint32			frame;
	EntityAtk*		geyser_sprout;
	// velocity for geyser represents how fast the water sprouts
}HazardData;

Entity* hazard_spawn(HazardType h_type, GFC_Vector2D position, const char* name);
Entity* hazard_dummy_spawn(SJson* data, HazardType h_type, GFC_Vector2D position);
void update_geyser_hurtbox(Entity* self);
void update_vortex_boundbox(Entity* self);

#endif
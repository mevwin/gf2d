#ifndef __COLLISIONS_H__
#define __COLLISIONS_H__

#include "level.h"

/**
* @brief check if entity is colliding with ground
* @note compare entity's bottom edge with nearest ground from current level's ground list
*/
Uint8 ground_collision(void* ent);

/**
* @brief check if entity is colliding with platform
* @note compare entity's bottom edge with nearest plat from current level's plat list
*/
Uint8 platform_collision(void* ent);

void handle_ent_plat_collision(void* e);

/**
* @brief check if entity is colliding with ground
* @param wall_type: collision with a wall; left == 0, right == 1
* @note compare entity's bottom edge with nearest ground from current level's ground list
*/
Uint8 wall_collision(void* ent, Uint8 wall_type);

/**
* @brief check if entity is colliding a ground's ceiling
* @note compare entity's top edge with nearest ground's bottom edge from current level's ground list
*/
Uint8 ceiling_collision(void* ent);

/**
* @brief return specified edge from rectangle
* @param box: the rect to pull an edge from
* @param side: left = 3, right = 2, top = 1, bottom = 0
*/
GFC_Edge2D get_edge_from_rect(GFC_Rect box, Uint8 side);

// Getters for colliding level objects
// @note: just simply calls 'find_nearest_thing' but should only be used when ent is close to the given obj
GFC_Rect get_colliding_ground(void* ent);
Wall* get_colliding_wall(void* ent, WallType wall_type);
Platform* get_colliding_plat(void* ent);
GFC_Edge2D get_colliding_ceiling(void* ent);

#endif
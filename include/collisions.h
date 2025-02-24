#ifndef __COLLISIONS_H_
#define __COLLISIONS_H_

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

/**
* @brief check if entity is colliding with ground
* @param wall_type: collision with a wall; left == 0, right == 1
* @note compare entity's bottom edge with nearest ground from current level's ground list
*/
Uint8 wall_collision(void* ent, Uint8 wall_type); // left = 0, right = 1

/**
* @brief check if entity is colliding a ground's ceiling
* @note compare entity's top edge with nearest ground's bottom edge from current level's ground list
*/
Uint8 ceiling_collision(void* ent);

/**
* @brief return specified edge from rectangle
* @param box: the rect to pull an edge from
* @param side: 3 = left, 2 = right, 1 = top, 0 = bottom
*/
GFC_Edge2D get_edge_from_rect(GFC_Rect box, Uint8 side);

#endif
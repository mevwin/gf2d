#ifndef __ENEMY_H__
#define __ENEMY_H__

#include "entity.h"

typedef enum EnemyType_E{
    BRUISER
}EnemyType;

typedef struct EnemyData_S{
    EnemyType   type;

    float			currHealth;
	float			maxHealth;
}EnemyData;

void enemy_spawn(EnemyType type, GFC_Vector2D position);

#endif
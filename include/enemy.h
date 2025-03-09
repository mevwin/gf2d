#ifndef __ENEMY_H__
#define __ENEMY_H__

#include "entity.h"

typedef enum EnemyType_E{
    BRUISER
}EnemyType;

//typedef struct EnemyAtk_S {

//}EnemyAtk;

typedef struct EnemyData_S{
    EnemyType       type;

    float			currHealth;
	float			maxHealth;
    float           dmg_reduction;
}EnemyData;

void enemy_spawn(int type, GFC_Vector2D position);

#endif
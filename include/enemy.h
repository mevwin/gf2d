#ifndef __ENEMY_H__
#define __ENEMY_H__

#include "entity.h"

typedef enum EnemyType_E{
    BRUISER,
    SLASHER,
    SPROUTER,
    CHASER
}EnemyType;

//typedef struct EnemyAtk_S {

//}EnemyAtk;

typedef struct EnemyData_S{
    EnemyType           type;

    float			    currHealth;
	float			    maxHealth;
    float               dmg_reduction;
    float               then;

    Uint8               item;

    GFC_Vector2D*       sprouter_spawns;
    Uint8               sprouter_spawn_index;
    Uint32              sprouter_idle_counter;
    Uint32              sprouter_idle_frames;
    Uint8               sprouter_spawns_count;
}EnemyData;

Entity* enemy_spawn(int type, const char* name, GFC_Vector2D position, SJson* sprouter_spawns);
Entity* enemy_dummy_spawn(SJson* data, EnemyType e_type, GFC_Vector2D position);

#endif
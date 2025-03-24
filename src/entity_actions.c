#include "simple_logger.h"
#include "world.h"
#include "collisions.h"
#include "player_attack.h"
#include "player.h"
#include "enemy.h"

typedef enum EntityDmgType_E {
    PLAYER_ENEMY,
    ENEMY_PLAYER,
    SELF_DAMAGE,
    PLAYER_HAZARD,
    HAZARD_PLAYER,
    HAZARD_ENEMY
}EntityDmgType;

void entity_damage(Entity* inflictor, Entity* recipient, EntityAtk* atk) {
    EntityDmgType dmg_type;
    PlayerData* p_data;
    EnemyData* e_data;
    // insert other data structs here

    if (!inflictor || !recipient) {
        //slog("somethings missing");
        return;
    }

    // determine who's who
    if (inflictor->type == PLAYER && recipient->type == ENEMY)
        dmg_type = PLAYER_ENEMY;
    else if (inflictor->type == ENEMY && recipient->type == PLAYER)
        dmg_type = ENEMY_PLAYER;
    else if (inflictor == recipient)
        dmg_type = SELF_DAMAGE;
    else if (inflictor->type == PLAYER && recipient->type == HAZARD)
        dmg_type = PLAYER_HAZARD;
    else if (inflictor->type == HAZARD && recipient->type == PLAYER)
        dmg_type = HAZARD_PLAYER;
    else
        dmg_type = HAZARD_ENEMY;

    // handle interaction
    switch (dmg_type) {
    case PLAYER_ENEMY:
        p_data = (PlayerData*)inflictor->data;
        e_data = (EnemyData*)recipient->data;
        if (!p_data || !e_data) return;

        e_data->currHealth -= atk->damage;
        //slog("enemy health: %f", e_data->currHealth);

        break;

    case ENEMY_PLAYER:


        break;

    case SELF_DAMAGE:

        break;

    case PLAYER_HAZARD:

        break;

    case HAZARD_PLAYER:

        break;

    case HAZARD_ENEMY:

        break;
    }
    atk->active = 0;
}

Entity* find_nearest_entity(Entity* source, void* check, EntitySearchType search) {
    Uint32 entityMax;
    Entity* entityList, * ent;
    EntityAtk* atk;
    GFC_Rect* hitbox;
    int i;

    if (!source || !check) return;

    entityMax = getEntityMax();
    entityList = getEntityList();
    hitbox = NULL;
    for (i = 0; i < entityMax; i++) {
        ent = &entityList[i];
        if (!ent->_inuse || source == ent) continue;

        switch (search) {
        case SEARCH_ATK:
            if (!ent->canBeDamaged) continue;

            atk = (EntityAtk*)check;
            hitbox = &atk->hitbox;

            break;

        case SEARCH_BASH:
            if (!ent->canBeBashed) continue;
            hitbox = (GFC_Rect*)check;

            break;

        }

        if (hitbox && gfc_rect_overlap(ent->hurtbox.s.r, *hitbox))
            return ent;
    }
    return NULL;
}
#ifndef __PLAYER_ATTACKS_H__
#define __PLAYER_ATTACKS_H__

typedef enum PlayerAtkState_E {
	PLAYER_ATK_NONE,
	PLAYER_ATK_STARTUP,
	PLAYER_ATK_ACTIVE,
	PLAYER_ATK_RECOVERY
}PlayerAtkState;

typedef enum PlayerAtkType_E {
	PLAYER_ATK_TYPE_NONE,
	PLAYER_ATK_TYPE_F_BASIC,
	PLAYER_ATK_TYPE_U_BASIC,
	PLAYER_ATK_TYPE_D_BASIC,
	PLAYER_ATK_TYPE_F_SPECIAL,
	PLAYER_ATK_TYPE_U_SPECIAL,
	PLAYER_ATK_TYPE_D_SPECIAL
}PlayerAtkType;

typedef struct PlayerAtk_S {
	PlayerAtkType		atk_type;
	GFC_Rect			hitbox;
	float				damage;
	Uint32				startupFrames;
	Uint32				activeFrames;
	Uint32				recovFrames;

	// extra things for later
	
}PlayerAtk;


void player_atk_system_init(void* data);

void player_attack(void* p, void* data);
#endif
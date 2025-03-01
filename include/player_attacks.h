#ifndef __PLAYER_ATTACKS_H__
#define __PLAYER_ATTACKS_H__

#include "gfc_input.h"

typedef enum PlayerAttackState_E {
	PLAYER_ATTACK_NONE,
	PLAYER_ATTACK_STARTUP,
	PLAYER_ATTACK_ACTIVE,
	PLAYER_ATTACK_RECOVERY
}PlayerAttackState;

#endif
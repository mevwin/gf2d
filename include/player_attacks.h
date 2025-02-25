#ifndef __PLAYER_ATTACKS_H
#define __PLAYER_ATTACKS_H

#include "gfc_input.h"

typedef enum PlayerAttackState_E {
	PLAYER_ATTACK_NONE,
	PLAYER_ATTACK_STARTUP,
	PLAYER_ATTACK_ACTIVE,
	PLAYER_ATTACK_RECOVERY
}PlayerAttackState;

#endif
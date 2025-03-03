#include "simple_logger.h"
#include "player.h"
#include "collisions.h"
#include "player_attacks.h"

typedef struct PlayerAttackManager_S {
	PlayerAttackState	atk_state;
	float				atk_end;

	PlayerAttackType	atk_type;
	float				damage;
	float				atk_dur;
	Uint8				active;
}PlayerAttackManager;

static PlayerAttackManager atk_manager = { 0 };

void player_atk_system_close();

void player_atk_system_init() {
	atk_manager.atk_state = PLAYER_ATTACK_NONE;

	atexit(player_atk_system_close);
}

void player_atk_system_close() {
	memset(&atk_manager, 0, sizeof(PlayerAttackManager));
}

void player_attack(void* p, void* data) {
	Entity* player;
	PlayerData* p_data;

	player = (Entity*) p;
	p_data = (PlayerData*) data;
	/*
	if (p_data->state != PLAYER_DODGE && p_data->state != PLAYER_BASH) {
		if (gfc_input_command_pressed("attack")) {

		}
		else if (gfc_input_command_pressed("special")) {

		}
	}
	*/
}
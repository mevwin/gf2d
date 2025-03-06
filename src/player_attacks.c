#include "simple_logger.h"
#include "gfc_config.h"
#include "gf2d_draw.h"
#include "world.h"
#include "player.h"
#include "collisions.h"
#include "player_attacks.h"

typedef struct PlayerAttackManager_S {
	GFC_List*			atk_list;
	PlayerAtk*			curr_atk;

	PlayerAtkState		atk_state;

	// timing
	float				frame_dur;
	float				then;

	// attack flags
	Uint8				active;			// deal damage once, turn off once damage has been dealt
	Uint8				aerial;

	// debug
	Uint8				frame;
}PlayerAttackManager;

static PlayerAttackManager atk_manager = { 0 };

void player_atk_system_close();
void create_player_atk(SJson* atk_data, Uint8 index);

void player_atk_system_init(void* data) {
	SJson* atk_list;
	int i;

	// save all player attacks as a list
	atk_list = (SJson*)data;
	if (!atk_list) {
		slog("no data provided");
		return;
	}

	atk_manager.atk_list = gfc_list_new();
	for (i = 0; i < atk_list->v.array->count; i++) {
		create_player_atk(sj_array_get_nth(atk_list, i), i);
	}

	atk_manager.frame_dur = 0.016f;
	atk_manager.frame = 0;
	atk_manager.atk_state = PLAYER_ATK_NONE;

	atexit(player_atk_system_close);
}

void player_atk_system_close() {
	gfc_list_foreach(atk_manager.atk_list, free);
	gfc_list_delete(atk_manager.atk_list);

	memset(&atk_manager, 0, sizeof(PlayerAttackManager));
}	

void create_player_atk(SJson* atk_data, Uint8 index) {
	GFC_Vector2D hitbox_dimen;
	PlayerAtk *atk;

	if (!atk_data) return;

	atk = gfc_allocate_array(sizeof(PlayerAtk), 1);
	if (!atk) {
		slog("failed to allocate space for atk");
		return;
	}

	atk->atk_type = (PlayerAtkType) (index + 1);
	sj_object_get_vector2d(atk_data, "hitbox", &hitbox_dimen);
	atk->hitbox = gfc_rect(0, 0, hitbox_dimen.x, hitbox_dimen.y);

	sj_object_get_float(atk_data, "damage", &atk->damage);

	sj_object_get_uint32(atk_data, "startupFrames", &atk->startupFrames);

	sj_object_get_uint32(atk_data, "activeFrames", &atk->activeFrames);
	atk->activeFrames += atk->startupFrames;

	sj_object_get_uint32(atk_data, "recovFrames", &atk->recovFrames);
	atk->recovFrames += atk->activeFrames;

	gfc_list_append(atk_manager.atk_list, atk);
}

void player_attack(void* p, void* data) {
	Entity* player;
	PlayerData* p_data;
	Uint8 atk_index;
	float time;

	player = (Entity*) p;
	p_data = (PlayerData*) data;

	if (!player || !p_data) return;

	// player should only attack during when idle, moving, or slowing down
	if (p_data->state != PLAYER_IDLE && p_data->state != PLAYER_MOVING && p_data->state != PLAYER_SLOWDOWN) return;

	time = CURRENT_TIME;

	switch (atk_manager.atk_state) {
		case PLAYER_ATK_STARTUP: // simply delaying until active
			if (time - atk_manager.then > atk_manager.frame_dur) {
				atk_manager.frame++;
				atk_manager.then = time;
				//slog("startup: %i", atk_manager.frame);
			}
		
			if (atk_manager.frame == atk_manager.curr_atk->startupFrames)
				atk_manager.atk_state = PLAYER_ATK_ACTIVE;

			break;

		case PLAYER_ATK_ACTIVE:
			/* display hitbox */
			// first shift rect coords based on player coords and type of move
			switch (atk_manager.curr_atk->atk_type) {
				case PLAYER_ATK_TYPE_F_BASIC:
					atk_manager.curr_atk->hitbox.x = player->position.x;
					atk_manager.curr_atk->hitbox.x += player->dir.x == 0 ? (player->hurtbox.s.r.w * 0.5f) : 
													-(player->hurtbox.s.r.w * 0.5f + atk_manager.curr_atk->hitbox.w);
					
					atk_manager.curr_atk->hitbox.y = player->position.y - (player->hurtbox.s.r.h * 0.5f);

					break;

				case PLAYER_ATK_TYPE_U_BASIC:
					atk_manager.curr_atk->hitbox.x = player->position.x - (atk_manager.curr_atk->hitbox.w * 0.5f);
					atk_manager.curr_atk->hitbox.y = player->position.y - (player->hurtbox.s.r.h * 0.5f) - (atk_manager.curr_atk->hitbox.h);

					//gf2d_draw_rect_filled(atk_manager.curr_atk->hitbox, GFC_COLOR_RED);

					break;
	
				case PLAYER_ATK_TYPE_D_BASIC:
					atk_manager.curr_atk->hitbox.x = player->position.x - (atk_manager.curr_atk->hitbox.w * 0.5f);
					atk_manager.curr_atk->hitbox.y = player->position.y + (player->hurtbox.s.r.h * 0.5f);

					//gf2d_draw_rect_filled(atk_manager.curr_atk->hitbox, GFC_COLOR_RED);

					break;
			}

			gf2d_draw_rect_filled(atk_manager.curr_atk->hitbox, gfc_color8(255, 0, 0,120));

			// frame checking
			if (time - atk_manager.then > atk_manager.frame_dur) {
				atk_manager.frame++;
				atk_manager.then = time;
				//slog("active: %i", atk_manager.frame);
			}

			if (atk_manager.frame == atk_manager.curr_atk->activeFrames) 
				atk_manager.atk_state = PLAYER_ATK_RECOVERY;			

			break;

		case PLAYER_ATK_RECOVERY:
			if (time - atk_manager.then > atk_manager.frame_dur) {
				atk_manager.frame++;
				atk_manager.then = time;
				//slog("recovery: %i", atk_manager.frame);
			}

			if (atk_manager.frame == atk_manager.curr_atk->recovFrames) {
				// reset manager values
				atk_manager.frame = 0;
				atk_manager.then = 0;
				atk_manager.atk_state = PLAYER_ATK_NONE;
			}

			break;

		default: //PLAYER_ATK_NONE GOAL: check player player input
			atk_index = PLAYER_ATK_TYPE_NONE;
			if (gfc_input_command_pressed("attack")) {
				if (gfc_input_command_down("moveup")) {
					slog("UP_TILT");
					atk_index = (Uint8) PLAYER_ATK_TYPE_U_BASIC;
					atk_manager.aerial = !ground_collision(player) ? 1 : 0;
				}
				else if (gfc_input_command_down("movedown")) {
					slog("DOWN_TILT");
					atk_index = (Uint8) PLAYER_ATK_TYPE_D_BASIC;
					atk_manager.aerial = !ground_collision(player) ? 1 : 0;
				}
				else { // forward by default
					slog("F_TILT");
					atk_index = (Uint8) PLAYER_ATK_TYPE_F_BASIC;
					atk_manager.aerial = !ground_collision(player) ? 1 : 0;
				}
			}
			else if (gfc_input_command_pressed("special")) {
				if (gfc_input_command_down("moveup")) {
					slog("UP_SPECIAL");
					atk_index = (Uint8) PLAYER_ATK_TYPE_U_SPECIAL;
					
				}
				else if (gfc_input_command_down("movedown")) {
					slog("DOWN_SPECIAL");
					atk_index = (Uint8) PLAYER_ATK_TYPE_D_SPECIAL;
					atk_manager.aerial = !ground_collision(player) ? 1 : 0;
				}
				else { // forward by default
					slog("FORWARD_SPECIAL");
					atk_index = (Uint8) PLAYER_ATK_TYPE_F_SPECIAL;
					atk_manager.aerial = !ground_collision(player) ? 1 : 0;
				}
			}
			
			//slog("atk_index: %i", atk_index - 1);
			if (atk_index) { // if attack has been set
				// set startup timer
				atk_manager.curr_atk = (PlayerAtk*) gfc_list_nth(atk_manager.atk_list, atk_index - 1);
				if (!atk_manager.curr_atk) {
					slog("no atk found at this index or atk is null");
					return;
				}
				atk_manager.then = time;

				// change state
				atk_manager.atk_state = PLAYER_ATK_STARTUP;
			}



	}
}
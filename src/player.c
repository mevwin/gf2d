#include <SDL.h>
#include "simple_logger.h"
#include "gfc_config.h"
#include "gf2d_draw.h"
#include "world.h"
#include "player_attack.h"
#include "collisions.h"
#include "player.h"

// required entity functions
void player_think(Entity* self);
void player_update(Entity* self);
void player_gravity(Entity* self);
void player_free(Entity* self);

/**
* @brief initialize PlayerData
* @param data: json data to initialize PlayerData
* @note SHOULD ONLY BE USED ONCE
*/
PlayerData* player_data_init(Entity* self, SJson* data);

static Entity* player;

Entity* player_spawn(GFC_Vector2D position, SJson* data) {
	SJson* curr_entry;

	player = entity_new();
	if (!player) {
		slog("failed to initialize player entity");
		return NULL;
	}

	player->type = PLAYER;
	gfc_line_cpy(player->name, "Player");	// change later

	curr_entry = sj_object_get_value(data, "entity_data");

	gfc_vector2d_copy(player->position, position);
	player->velocity = gfc_vector2d(0, 0);
	sj_object_get_vector2d(curr_entry, "max_velocity", &player->max_velocity);
	sj_object_get_vector2d(curr_entry, "accel", &player->accel);
	player->sprite = gf2d_sprite_load_image(sj_object_get_string(curr_entry, "sprite"));
	player->frame = 0;

	player->think = player_think;
	player->update = player_update;
	player->grav = player_gravity;
	player->grav_flag = 1;

	player->scale = gfc_vector2d(1, 1);
	player->dir = gfc_vector2d(0, 0);
	player->free = player_free;

	update_hurtbox(player);
	update_boundbox(player);

	player->data = player_data_init(player, sj_object_get_value(data, "player_data"));
	if (!player->data) {
		slog("failed to initialize player data");
		return NULL;
	}

	player_atk_system_init(sj_object_get_value(data, "player_attacks"));

	return player;
}

void player_free(Entity* self) {
	gf2d_sprite_delete(self->sprite);
	
	if (self->data) free(self->data);
}

PlayerData* player_data_init(Entity* self, SJson* data) {
	PlayerData* p_data;
	SJson *ability_checks;

	p_data = gfc_allocate_array(sizeof(PlayerData), 1);
	if (!p_data) return NULL;

	// default values
	gfc_vector2d_copy(p_data->spawn_pos, self->position);
	p_data->state = PLAYER_IDLE;
	p_data->moveTypeX = PMOVE_NONE_X;
	p_data->moveTypeY = PMOVE_NONE_Y;
	p_data->jump_count = 0;

	// player stats
	sj_object_get_float(data, "maxHealth", &p_data->maxHealth);
	p_data->currHealth = p_data->maxHealth;

	// ability checks
	ability_checks = sj_object_get_value(data, "ability_checks");
	sj_object_get_uint8(ability_checks, "canRecall", &p_data->canRecall);
	sj_object_get_uint8(ability_checks, "canDoubleJump", &p_data->canDoubleJump);
	sj_object_get_uint8(ability_checks, "canBash", &p_data->canBash);
	sj_object_get_uint8(ability_checks, "canWallJump", &p_data->canWallJump);
	sj_object_get_uint8(ability_checks, "canDodge", &p_data->canDodge);

	if (p_data->canRecall) player_recall_init();
	if (p_data->canBash) player_bash_init();

	// movement values
	sj_object_get_uint8(data, "max_jumps", &p_data->max_jumps);
	sj_object_get_uint8(data, "max_dodge_charges", &p_data->max_dodge_charges);
	p_data->dodge_charges = p_data->max_dodge_charges;
	sj_object_get_vector2d(data, "dodge_vel", &p_data->dodge_vel);
	sj_object_get_float(data, "dodge_vel_reduc", &p_data->dodge_vel_reduc);
	sj_object_get_float(data, "jump_speed", &p_data->jump_speed);
	sj_object_get_vector3d(data, "friction", &p_data->friction);

	return p_data;
}

void player_think(Entity* self) {
	PlayerData* p_data;

	p_data = self->data;

	player_move(self);
	player_attack(self, p_data);
}

void player_update(Entity* self) {
	PlayerData* p_data;
	//float ground_level;

	p_data = self->data;
	if (!p_data) return;

	if (!p_data->isAttacking) {
		switch (p_data->moveTypeX) {
			case PMOVE_LEFT:
				self->dir.x = 1;
				break;
			case PMOVE_RIGHT:
				self->dir.x = 0;
				break;
			}
	}

	// update player based on movement
	if (p_data->canRecall) track_player(self, p_data);
	// special movement abilties
	if (p_data->state == PLAYER_RECALL) player_recall(self, p_data);
	else if (p_data->state == PLAYER_BASH) player_bash(self, p_data);

	// dummy respawn
	if (self->position.y > RES.y + RES.h) {
		player_recall_reset(self, p_data, CURRENT_TIME);
		gfc_vector2d_copy(self->position, p_data->spawn_pos);
	}

	// 
	//if (platform_collision(self)) handle_

	/*DEBUG: center checking*/
	//gf2d_draw_rect(self->boundbox.s.r, GFC_COLOR_RED);

	/*
	gf2d_draw_line(
		gfc_vector2d(bottom.x1, bottom.y1),
		gfc_vector2d(bottom.x2, bottom.y2),
		GFC_COLOR_RED
	);*/
	
	/*
	gf2d_draw_line(
		gfc_vector2d(0, self->position.y),
		gfc_vector2d(1200, self->position.y),
		GFC_COLOR_BLUE
	);
	gf2d_draw_line(
		gfc_vector2d(self->position.x, 0),
		gfc_vector2d(self->position.x, 720),
		GFC_COLOR_BLUE
	);
	*/
}

void player_gravity(Entity* self) {
	PlayerData* p_data;

	p_data = self->data;
	if (!p_data) return;
	if (p_data->state == PLAYER_DODGE) return;

	if ((ground_collision(self) || platform_collision(self)) && !p_data->jump_count) { // grounded
		if (self->velocity.x == 0)
			p_data->turnaround = 0;

		if (self->velocity.x > 0)
			p_data->state = PLAYER_SLOWDOWN;

		if (!self->plat_flag)
			self->plat_flag = 1;
	}
	else if ((ground_collision(self) || platform_collision(self)) && p_data->jump_count) { // landing
		// reset some flags and values as needed before landing
		p_data->dodge_charges = p_data->max_dodge_charges;
		p_data->jump_count = 0;
		self->velocity.y = 0;
		p_data->wall_jump = 0;
		p_data->moveTypeY = PMOVE_NONE_Y;
	}
	else { // falling
		// change moveTypeY to FALLING once peak of jump has reached
		if (p_data->moveTypeY == PMOVE_RISING &&
			(ceiling_collision(self) || self->velocity.y < 1.0f && self->velocity.y > -2.0f)
			) {
			p_data->moveTypeY = PMOVE_FALLING;
			self->velocity.y = 0;
		}

		self->position.y -= self->velocity.y;

		// increase falling speed
		if (self->velocity.y < -self->max_velocity.y) // limit vertical velocity
			self->velocity.y = -self->max_velocity.y;
		else
			self->velocity.y -= p_data->moveTypeY == PMOVE_FASTFALLING ? self->accel.y : GRAVITY;

		// reset wall jump
		if (self->velocity.y <= 7.0f) 
			p_data->wall_jump = 0;

		// prevent MultiVersus DJ
		if (!p_data->jump_count)
			p_data->jump_count++;
	}
}

GFC_Vector2D* get_player_pos() {
	return &player->position;
}
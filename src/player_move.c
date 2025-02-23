#include "simple_logger.h"
#include "world.h"
#include "player.h"
#include "collisions.h"

typedef struct PlayerRecall_S {
	// timing
	float	cooldown_time;
	float   next_recall;


}PlayerRecall;

static PlayerRecall recall_manager = { 0 };

void player_move(void* p) {
	PlayerData* p_data;
	Entity* self;
	Uint8 i;

	self = (Entity*) p;
	if (!self) return;

	p_data = self->data;
	if (!p_data) return;

	/* BASE HORIZONTAL MOVEMENT */
	// to help maintain momentum in the air
	if (!gfc_input_command_pressed("moveright") && !gfc_input_command_pressed("moveleft")
		&& self->velocity.x > 0 && !ground_collision(self) && p_data->state != DODGE)
		p_data->state = MOVING;

	if ((gfc_input_command_released("moveleft") || gfc_input_command_released("moveright"))
		&& self->velocity.x > 0 && p_data->state != DODGE)
		p_data->state = SLOWDOWN;

	// input checks
	if ((gfc_input_command_down("moveleft") || gfc_input_command_pressed("moveleft"))
		&& !gfc_input_command_pressed("moveright") && p_data->state != DODGE && !p_data->wall_jump) {
		p_data->state = MOVING;

		if (p_data->moveTypeX == PMOVE_RIGHT && self->velocity.x > 0) {
			p_data->state = SLOWDOWN;
			if (ground_collision(self))
				p_data->turnaround = 1;
		}
		else
			p_data->moveTypeX = PMOVE_LEFT;

		if (self->velocity.x <= self->max_velocity.x) // ground
			self->velocity.x += self->accel.x;
		else if (p_data->turnaround && self->velocity.x <= self->max_velocity.x) // air
			self->velocity.x += self->accel.x * 2.0f;
	}
	else if ((gfc_input_command_down("moveright") || gfc_input_command_pressed("moveright"))
		&& !gfc_input_command_pressed("moveleft") && p_data->state != DODGE && !p_data->wall_jump) {
		p_data->state = MOVING;

		if (p_data->moveTypeX == PMOVE_LEFT && self->velocity.x > 0) {
			p_data->state = SLOWDOWN;
			if (ground_collision(self))
				p_data->turnaround = 1;
		}
		else
			p_data->moveTypeX = PMOVE_RIGHT;

		if (self->velocity.x <= self->max_velocity.x) // ground
			self->velocity.x += self->accel.x;
		else if (p_data->turnaround && self->velocity.x <= self->max_velocity.x) // air
			self->velocity.x += self->accel.x * 2.0f;
	}

	/* DODGE */
	if ((gfc_input_command_pressed("dodge")) && p_data->state != DODGE && p_data->dodge_charges > 0) {
		if (gfc_input_command_pressed("moveright") || self->dir.x == 0)
			p_data->moveTypeX = PMOVE_RIGHT;
		else if (gfc_input_command_pressed("moveleft") || self->dir.x == 1)
			p_data->moveTypeX = PMOVE_LEFT;

		p_data->state = DODGE;
		p_data->moveTypeY = PMOVE_NONE_Y;

		self->velocity.x = p_data->dodge_vel.x;
		if (!ground_collision(self)) {
			p_data->dodge_charges--;
			self->velocity.x = p_data->dodge_vel.y;
		}
		self->velocity.y = 0;
	}

	// 0 == left wall, 1 == right wall
	i = self->dir.x == 0 ? 0 : 1;

	/* BASE VERTICAL MOVEMENT (NOTE: VERTICAL MEANS NEGATIVE Y)*/
	/* JUMP */
	if (gfc_input_command_pressed("jump")) {
		if (!wall_collision(self, i) && p_data->jump_count < p_data->max_jumps) {
			self->velocity.y = p_data->jump_speed;

			p_data->jump_count++;
			p_data->moveTypeY = PMOVE_RISING;
		}
		else if (wall_collision(self, i) && !p_data->wall_jump && p_data->jump_count > 0) {
			p_data->wall_jump = 1;
			self->velocity.y = p_data->jump_speed;
			self->velocity.x = self->max_velocity.x;
			p_data->moveTypeX = i ? PMOVE_RIGHT : PMOVE_LEFT; // if wall jumping from right, go left (and vice versa)
			p_data->moveTypeY = PMOVE_RISING;
		}
	}

	if (p_data->moveTypeY == PMOVE_FALLING && (gfc_input_command_pressed("movedown") || gfc_input_command_down("movedown"))) {
		// remember at this point, velocity.y is negative
		p_data->moveTypeY = PMOVE_FASTFALLING;
	}

	// big-ass state check to actually apply the movement
	if (p_data->state == MOVING) { // regular movement
		if ((wall_collision(self, i) && !p_data->wall_jump) || entity_keep_in_bounds(self, i)) {
			self->velocity.x = 0;
			return;
		}

		if (self->velocity.x > self->max_velocity.x)
			self->velocity.x -= p_data->friction.z;

		self->position.x += p_data->moveTypeX == PMOVE_RIGHT ?
			self->velocity.x : -self->velocity.x;
	}
	else if (p_data->state == SLOWDOWN) {
		if (wall_collision(self, i) || entity_keep_in_bounds(self, i)) {
			self->velocity.x = 0;
			return;
		}

		// sliding effect
		self->position.x += p_data->moveTypeX == PMOVE_RIGHT ?
			self->velocity.x : -self->velocity.x;

		// if on ground, apply friction
		if (ground_collision(self) && self->velocity.x > 0) {
			self->velocity.x -= p_data->turnaround ? p_data->friction.x : p_data->friction.y;

			if (self->velocity.x < 0) {
				self->velocity.x = 0;
				p_data->moveTypeX = PMOVE_NONE_X;
				p_data->state = IDLE;
			}
		}
		else { // if in air and turning around, slow down
			self->velocity.x -= p_data->friction.x;

			if (self->velocity.x < 0) {
				p_data->moveTypeX = p_data->moveTypeX != PMOVE_RIGHT ? PMOVE_RIGHT : PMOVE_LEFT;
				p_data->turnaround = 1;
				self->velocity.x = 0;
			}
		}
	}
	else if (p_data->state == DODGE) {
		if (entity_keep_in_bounds(self, i) || wall_collision(self, i)) {
			self->velocity.x = 0;
			return;
		}

		self->position.x += p_data->moveTypeX == PMOVE_RIGHT ?
			self->velocity.x : -self->velocity.x;

		self->velocity.x -= p_data->dodge_vel_reduc;

		// dodge jump momentum carrying
		if (!ground_collision(self) && gfc_input_command_pressed("jump"))
			p_data->state = MOVING;
		else if (self->velocity.x < 0.4f * p_data->dodge_vel.y) {
			self->velocity.x = 0.4f * p_data->dodge_vel.y;
			p_data->state = SLOWDOWN;
		}
	}
}

void track_player(void* p) {
	PlayerData* p_data;
	Entity* self;
}

void player_recall(void* p) {
	PlayerData* p_data;
	Entity* self;
}
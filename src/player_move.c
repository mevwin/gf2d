#include "simple_logger.h"
#include "world.h"
#include "player.h"
#include "collisions.h"

typedef struct PlayerRecall_S {
	// timing
	float				cooldown_time;
	float				next_recall;

	/*' player-tracking' data */
	GFC_List*			pathToPlayer;
	int					pathToPlayer_index;
	RecallPoint*		curr_point;

	GFC_Vector2D		rewind_velocity;
	GFC_Vector2D		rewind_accel;

	float				total_distance;
	float				time_to_record;
	float				next_recording_time;

	RecallState		state;
}RecallManager;

void player_recall_close();

void move_player_to_recall_pos(Entity* player);

float calc_total_recall_distance(GFC_Vector2D initial);

static RecallManager recall_manager = { 0 };

void player_recall_init() {
	recall_manager.cooldown_time = 3.0f;
	recall_manager.next_recall = CURRENT_TIME;
	recall_manager.state = RECALL_NONE;
	recall_manager.pathToPlayer = gfc_list_new();
	recall_manager.time_to_record = 0.2f;
	recall_manager.next_recording_time = CURRENT_TIME + recall_manager.time_to_record;

	atexit(player_recall_close);
}

void player_recall_close() {
	gfc_list_foreach(recall_manager.pathToPlayer, free);
	gfc_list_delete(recall_manager.pathToPlayer);
	memset(&recall_manager, 0, sizeof(RecallManager));
}

void player_move(void* p) {
	PlayerData* p_data;
	Entity* self;
	Uint8 i;

	self = (Entity*) p;
	if (!self) return;

	p_data = self->data;
	if (!p_data || p_data->state == PLAYER_RECALL) return;

	/* BASE HORIZONTAL MOVEMENT */
	// to help maintain momentum in the air
	if (!gfc_input_command_down("moveright") && !gfc_input_command_down("moveleft")
		&& self->velocity.x > 0 && !ground_collision(self) && !platform_collision(self) && p_data->state != PLAYER_DODGE)
		p_data->state = PLAYER_MOVING;

	if ((gfc_input_command_released("moveleft") || gfc_input_command_released("moveright"))
		&& self->velocity.x > 0 && p_data->state != PLAYER_DODGE)
		p_data->state = PLAYER_SLOWDOWN;

	// input checks
	if ((gfc_input_command_down("moveleft") || gfc_input_command_pressed("moveleft"))
		&& !gfc_input_command_pressed("moveright") && p_data->state != PLAYER_DODGE && !p_data->wall_jump) {
		p_data->state = PLAYER_MOVING;

		if (p_data->moveTypeX == PMOVE_RIGHT && self->velocity.x > 0) {
			p_data->state = PLAYER_SLOWDOWN;
			if (ground_collision(self) || platform_collision(self))
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
		&& !gfc_input_command_pressed("moveleft") && p_data->state != PLAYER_DODGE && !p_data->wall_jump) {
		p_data->state = PLAYER_MOVING;

		if (p_data->moveTypeX == PMOVE_LEFT && self->velocity.x > 0) {
			p_data->state = PLAYER_SLOWDOWN;
			if (ground_collision(self) || platform_collision(self))
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
	if (p_data->canDodge && gfc_input_command_pressed("dodge") && p_data->state != PLAYER_DODGE && p_data->dodge_charges > 0) {
		if (gfc_input_command_pressed("moveright") || self->dir.x == 0)
			p_data->moveTypeX = PMOVE_RIGHT;
		else if (gfc_input_command_pressed("moveleft") || self->dir.x == 1)
			p_data->moveTypeX = PMOVE_LEFT;

		p_data->state = PLAYER_DODGE;
		p_data->moveTypeY = PMOVE_NONE_Y;

		self->velocity.x = p_data->dodge_vel.x;
		if (!ground_collision(self) && !platform_collision(self)) {
			p_data->dodge_charges--;
			self->velocity.x = p_data->dodge_vel.y;
		}
		self->velocity.y = 0;
	}

	if (p_data->canRecall && gfc_input_command_pressed("recall")){
		if (CURRENT_TIME >= recall_manager.next_recall) {
			recall_manager.state = RECALL_START;
			p_data->state = PLAYER_RECALL;
			return;
		}
		else
			slog("waiting");
	}

	// 0 == left wall, 1 == right wall
	i = self->dir.x == 0 ? 0 : 1;

	/* BASE VERTICAL MOVEMENT (NOTE: VERTICAL MEANS NEGATIVE Y)*/
	/* JUMP */
	if (gfc_input_command_pressed("jump")) {
		// item upgrade checks
		if (!p_data->canDoubleJump) p_data->max_jumps = 1;
		if (!p_data->canWallJump) p_data->wall_jump = 1;

		if (!wall_collision(self, i) && p_data->jump_count < p_data->max_jumps) {
			self->velocity.y = p_data->jump_speed;

			p_data->jump_count++;
			p_data->moveTypeY = PMOVE_RISING;

			if (!self->plat_flag)
				self->plat_flag = 1;
		}
		else if (wall_collision(self, i) && !p_data->wall_jump && p_data->moveTypeY != PMOVE_NONE_Y) { // wall_jump
			p_data->wall_jump = 1;
			self->velocity.y = p_data->jump_speed;
			self->velocity.x = self->max_velocity.x;
			p_data->moveTypeX = i ? PMOVE_RIGHT : PMOVE_LEFT; // if wall jumping from right, go left (and vice versa)
			p_data->moveTypeY = PMOVE_RISING;

			if (!self->plat_flag)
				self->plat_flag = 1;
		}
	}

	if (p_data->moveTypeY == PMOVE_FALLING && (gfc_input_command_pressed("movedown") || gfc_input_command_down("movedown"))) {
		// remember at this point, velocity.y is negative
		p_data->moveTypeY = PMOVE_FASTFALLING;
	}

	/* MOVE THROUGH PLATFORM */
	if (p_data->moveTypeY == PMOVE_FASTFALLING || 
		(platform_collision(self) && (gfc_input_command_pressed("movedown") || gfc_input_command_down("movedown")))
		) {
		self->plat_flag = 0;
	}

	/* big ass state check to actually apply the movement */
	if (p_data->state == PLAYER_MOVING) { // regular movement
		if ((wall_collision(self, i) && !p_data->wall_jump) || entity_keep_in_bounds(self, i)) {
			self->velocity.x = 0;
			return;
		}

		if (self->velocity.x > self->max_velocity.x)
			self->velocity.x -= p_data->friction.z;

		self->position.x += p_data->moveTypeX == PMOVE_RIGHT ?
			self->velocity.x : -self->velocity.x;
	}
	else if (p_data->state == PLAYER_SLOWDOWN) {
		if (wall_collision(self, i) || entity_keep_in_bounds(self, i)) {
			self->velocity.x = 0;
			return;
		}

		// sliding effect
		self->position.x += p_data->moveTypeX == PMOVE_RIGHT ?
			self->velocity.x : -self->velocity.x;

		// if on ground, apply friction
		if ((ground_collision(self) || platform_collision(self)) && self->velocity.x > 0) {
			self->velocity.x -= p_data->turnaround ? p_data->friction.x : p_data->friction.y;

			if (self->velocity.x < 0) {
				self->velocity.x = 0;
				p_data->moveTypeX = PMOVE_NONE_X;
				p_data->state = PLAYER_IDLE;
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
	else if (p_data->state == PLAYER_DODGE) {
		if (entity_keep_in_bounds(self, i) || wall_collision(self, i)) {
			self->velocity.x = 0;
			return;
		}

		self->position.x += p_data->moveTypeX == PMOVE_RIGHT ?
							self->velocity.x : -self->velocity.x;

		self->velocity.x -= p_data->dodge_vel_reduc;

		// dodge jump momentum carrying
		if (!ground_collision(self) && !platform_collision(self) && gfc_input_command_pressed("jump"))
			p_data->state = PLAYER_MOVING;
		else if (self->velocity.x < 0.4f * p_data->dodge_vel.y) {
			self->velocity.x = 0.4f * p_data->dodge_vel.y;
			p_data->state = PLAYER_SLOWDOWN;
		}
	}
}

void track_player(void* p, void* data) {
	PlayerData* p_data;
	Entity* player;
	//RecallPosition* pos;

	player = (Entity*)p;
	p_data = (PlayerData*)data;
	if (!player || !p_data || recall_manager.state != RECALL_NONE) return;

	if (CURRENT_TIME >= recall_manager.next_recording_time) {
		gfc_list_append(recall_manager.pathToPlayer, create_recall_pos(player->position, CURRENT_TIME));
		recall_manager.next_recording_time = CURRENT_TIME + recall_manager.time_to_record;
		slog("saved position");
	}
}
	

void player_recall(void* p, void* data) {
	PlayerData* p_data;
	Entity* player;
	float recall_speed_mag;

	player = (Entity*)p;
	p_data = (PlayerData*)data;
	if (!player || !p_data) return;
	
	if (!recall_manager.pathToPlayer->count) { 
		recall_manager.state = RECALL_NONE;
		return; 
	}

	switch (recall_manager.state) {
		case RECALL_START: // prepare player and recall_manager for recall
			slog("recall stared: %i positions", recall_manager.pathToPlayer->count);
			recall_manager.pathToPlayer_index = (int)recall_manager.pathToPlayer->count - 1;

			// stop current player movement
			gfc_vector2d_clear(player->velocity);
			
			// initialize rewind values
			recall_manager.rewind_accel = gfc_vector2d(0.1f, 0.1f);
			recall_manager.curr_point = gfc_list_nth(recall_manager.pathToPlayer, recall_manager.pathToPlayer_index);
			recall_manager.total_distance = calc_total_recall_distance(player->position);

			// calculate move speed
			recall_speed_mag = gfc_vector2d_magnitude_between(player->position, recall_manager.curr_point->point) 
								/ recall_manager.total_distance;

			// change state
			player->grav_flag = 0;
			recall_manager.state = RECALL_REWIND;
			break;

		case RECALL_REWIND: // move player to next point	
			slog("%i", recall_manager.pathToPlayer_index);

			gfc_vector2d_move_towards(&player->position, player->position, recall_manager.curr_point->point,
									gfc_vector2d_magnitude_between(player->position, recall_manager.curr_point->point));
			//move_player_to_recall_pos(player);

			recall_manager.state = RECALL_STOP;

			break;

		case RECALL_STOP: // initialize next rewind attempt
			// check to see if player has reached that position
			if (gfc_vector2d_distance_between_less_than(player->position, recall_manager.curr_point->point, 3.0f))
				recall_manager.pathToPlayer_index--; // move to next point in list

			if (recall_manager.pathToPlayer_index < 0) // if reached last position
				recall_manager.state = RECALL_FINISH;
			else {
				recall_manager.state = RECALL_REWIND;
				recall_manager.curr_point = gfc_list_nth(recall_manager.pathToPlayer, recall_manager.pathToPlayer_index);
			
				// calculate next move speed;
				recall_speed_mag = gfc_vector2d_magnitude_between(player->position, recall_manager.curr_point->point)
					/ recall_manager.total_distance;
			}

			break;

		case RECALL_FINISH:
			gfc_list_foreach(recall_manager.pathToPlayer, free);
			gfc_list_clear(recall_manager.pathToPlayer);

			recall_manager.next_recording_time = CURRENT_TIME + 1.0f;
			recall_manager.next_recall = CURRENT_TIME + recall_manager.cooldown_time;

			player->grav_flag = 1;
			p_data->state = PLAYER_IDLE;

			recall_manager.state = RECALL_NONE;

			slog("finished");
			break;
	}
}

RecallPoint* create_recall_pos(GFC_Vector2D pos, float time) {
	RecallPoint* rp;

	rp = gfc_allocate_array(sizeof(RecallPoint), 1);
	gfc_vector2d_copy(rp->point, gfc_vector2d(pos.x, pos.y));
	rp->time = time;
	
	return rp;
}

void move_player_to_recall_pos(Entity* player) {
	GFC_Vector2D recall_vel;

	if (!player || !recall_manager.curr_point) return;



	// change velocity direction if needed
	/*
	player->position.x += pos->point.x > player->position.x ? 
						  recall_manager.rewind_velocity.x : -recall_manager.rewind_velocity.x;

	player->position.y += pos->point.y > player->position.y ?
						  recall_manager.rewind_velocity.y : -recall_manager.rewind_velocity.y;
	*/
}

float calc_total_recall_distance(GFC_Vector2D initial) {
	RecallPoint* rpf, *rpi;
	int i;
	float dist;

	if (!recall_manager.pathToPlayer) return 0.0f;
	
	dist = 0.0f;
	for (i = recall_manager.pathToPlayer->count - 1; i > 0; i--) {
		rpf = (RecallPoint*) gfc_list_nth(recall_manager.pathToPlayer, i);
		rpi = (RecallPoint*) gfc_list_nth(recall_manager.pathToPlayer, i - 1);
		if (!rpf || !rpi) continue;

		dist += gfc_vector2d_magnitude_between(rpf->point, rpi->point);
	}
	rpi = (RecallPoint*)gfc_list_nth(recall_manager.pathToPlayer, 0);
	dist += gfc_vector2d_magnitude_between(initial, rpi->point);
	return dist;
}
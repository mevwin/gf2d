#include "simple_logger.h"
#include "world.h"
#include "player.h"
#include "collisions.h"

typedef struct RecallManager_S {
	// timing
	float				cooldown_time;			// cooldown duration after recalling
	float				next_recall;			// time until next recall attempt

	/*' player-tracking' data */
	Uint8				max_positions;			// max number of previous recall points	
	int					pathToPlayer_index;		// pointer to current index in pathToPlayer
	GFC_List*			pathToPlayer;			// list of previous positions to reposition to

	RecallState			state;					// current state in recall process
	GFC_Vector2D		recall_dir;				// direction to recall to
	float				rewind_velocity_mag;	// rewind speed
	float				rewind_max_vel_mag;		// max rewind speed
	float				rewind_accel_mag;		// rate of change of rewind speed

	//float				total_distance;			// MEV_NOTE: old implementation used total_distance, commented out for later use?
	float				time_to_record;			// time to wait for recording next player position
	float				next_recording_time;	// timestamp to record next player position
}RecallManager;

typedef struct BashManager_S {
	BashState			state;					// current state of bashing process
	BashDir				bash_dir;				// direciton to bash to
	GFC_Vector2D		bash_vel;				
	float				bash_velocity_mag;
	float				bash_decel_mag;

	float				bashing_duration;		// allotted time to wait for an input
	float				bashing_stop_time;

	// for collision detection
	Uint8				collided;
	WallType			wall_type;
	void*				collided_obj;
	GFC_Rect			collided_ground;
	GFC_Edge2D			collided_ceil;
}BashManager;

static RecallManager recall_manager = { 0 };
static BashManager bash_manager = { 0 };

// recall functions
/**
* @brief close recall manager
*/
void player_recall_close();

/**
* @brief initalize a RecallPoint
* @param pos: position to rewind player back to
* @return pointer to a GFC_Vector2D representing the player's previous position at a previous point in time
*/
GFC_Vector2D* create_recall_pos(GFC_Vector2D pos);

/**
* @brief keeps the list of previous recall points set to max_positions (done by deleting index 0 in given list)
* @param list: list of old positions
*/
void trim_pathToPlayer(GFC_List* list);

// MEV_NOTE: old implementation used total_distance, commented out for later use?
//float calc_total_recall_distance(GFC_Vector2D initial);

// bash functions
/**
* @brief close bash manager
*/
void player_bash_close();

void player_recall_init() {
	// MEV_NOTE: maybe makes this driven in a config file
	float time;

	time = CURRENT_TIME;
	recall_manager.cooldown_time = 6.0f;
	recall_manager.next_recall = time;
	recall_manager.state = RECALL_NONE;
	recall_manager.max_positions = 10;
	recall_manager.pathToPlayer = gfc_list_new_size(recall_manager.max_positions);
	recall_manager.time_to_record = 0.2f;
	recall_manager.next_recording_time = time + recall_manager.time_to_record;
	
	atexit(player_recall_close);
}

void player_recall_close() {
	gfc_list_foreach(recall_manager.pathToPlayer, free);
	gfc_list_delete(recall_manager.pathToPlayer);
	memset(&recall_manager, 0, sizeof(RecallManager));
}

void player_bash_init() {
	// MEV_NOTE: maybe makes this driven in a config file
	bash_manager.state = BASH_NONE;
	bash_manager.bash_velocity_mag = 12.0f;
	bash_manager.bash_decel_mag = 0.5f;

	bash_manager.bashing_duration = 3.0f;
	//bash_manager.bashing_stop_time = CURRENT_TIME + bash_manager.bashing_duration;

	atexit(player_bash_close);
}

void player_bash_close() {
	memset(&bash_manager, 0, sizeof(BashManager));
}

// TODO: FIX THIS SHIT BY ADDING A DEDICATED MANAGER
void player_move(void* p) {
	PlayerData* p_data;
	Entity* self;
	Uint8 i;

	self = (Entity*) p;
	if (!self) return;

	p_data = self->data;
	if (!p_data || p_data->state == PLAYER_RECALL || p_data->state == PLAYER_BASH) return;

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

	/* RECALL */
	if (p_data->canRecall && gfc_input_command_pressed("recall")){
		if (CURRENT_TIME >= recall_manager.next_recall) {
			recall_manager.state = RECALL_START;
			p_data->state = PLAYER_RECALL;
			return;
		}
		else
			slog("waiting");
	}

	/* BASH */
	if (p_data->canBash && !ground_collision(self) && p_data->state != PLAYER_BASH && gfc_input_command_pressed("bash")) {
		//if (CURRENT_TIME >= bash_manager.next_bash) { // if next to a bash-able object
			bash_manager.state = BASH_START;
			p_data->state = PLAYER_BASH;
			return;
		//}
		//else
			//slog("waiting");
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

	/* FAST FALLING */
	// TODO: double tapping down for fast-falling
	/* 3/5/25: turn off for atk testing
	if (p_data->moveTypeY == PMOVE_FALLING && (gfc_input_command_pressed("movedown") || gfc_input_command_down("movedown"))) {
		// remember at this point, velocity.y is negative
		p_data->moveTypeY = PMOVE_FASTFALLING;
	}
	*/

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
			p_data->state = PLAYER_IDLE;
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
			p_data->state = PLAYER_IDLE;
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
			p_data->state = PLAYER_IDLE;
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
		if (recall_manager.pathToPlayer->count > recall_manager.max_positions) 
			trim_pathToPlayer(recall_manager.pathToPlayer);

		gfc_list_append(recall_manager.pathToPlayer, create_recall_pos(player->position));
		recall_manager.next_recording_time = CURRENT_TIME + recall_manager.time_to_record;
		//slog("saved position");
	}
}

// TODO: ADD FUNCTIONALITY FOR PLAYER RESOURCE RESTORATION
void player_recall(void* p, void* data) {
	PlayerData* p_data;
	Entity* player;
	GFC_Vector2D rp, *curr_point;
	float time;

	player = (Entity*)p;
	p_data = (PlayerData*)data;
	if (!player || !p_data) return;
	
	time = CURRENT_TIME;
	if (!recall_manager.pathToPlayer->count) { 
		recall_manager.state = RECALL_NONE;
		recall_manager.next_recording_time = time + 1.0f;
		recall_manager.next_recall = time + recall_manager.cooldown_time;
		slog("no recorded positions");
		return; 
	}

	switch (recall_manager.state) {
		case RECALL_START: // prepare player and recall_manager for recall
			//slog("recall stared: %i positions", recall_manager.pathToPlayer->count);
			recall_manager.pathToPlayer_index = (int)recall_manager.pathToPlayer->count - 1;

			// stop current player movement
			gfc_vector2d_clear(player->velocity);
			player->grav_flag = 0;
			player->plat_flag = 0;

			// initialize rewind values
			//recall_manager.total_distance = calc_total_recall_distance(player->position);
			recall_manager.rewind_accel_mag = 0.4f;
			recall_manager.rewind_velocity_mag = 1;
			recall_manager.rewind_max_vel_mag = 16.0f;
			curr_point = gfc_list_nth(recall_manager.pathToPlayer, recall_manager.pathToPlayer_index);
			if (!curr_point) {
				slog("recall point at index %i is null", recall_manager.pathToPlayer_index);
				recall_manager.state = RECALL_FINISH;
				return;
			}

			// calculate move direction
			rp = *curr_point;
			gfc_vector2d_sub(recall_manager.recall_dir, rp, player->position);
			gfc_vector2d_set_magnitude(&recall_manager.recall_dir, recall_manager.rewind_velocity_mag);

			// change state
			recall_manager.state = RECALL_REWIND;
			break;

		case RECALL_REWIND: // move player to next point	
			//slog("%i", recall_manager.pathToPlayer_index);
			gfc_vector2d_add(player->position, player->position, recall_manager.recall_dir);

			recall_manager.rewind_velocity_mag += recall_manager.rewind_accel_mag;
			if (recall_manager.rewind_velocity_mag > recall_manager.rewind_max_vel_mag)
				recall_manager.rewind_velocity_mag = recall_manager.rewind_max_vel_mag;
			recall_manager.state = RECALL_STOP;

			break;

		case RECALL_STOP: // initialize next rewind attempt
			curr_point =  gfc_list_nth(recall_manager.pathToPlayer, recall_manager.pathToPlayer_index);
			if (!curr_point) {
				slog("recall point at index %i is null", recall_manager.pathToPlayer_index);
				recall_manager.state = RECALL_FINISH;
				return;
			}

			// check to see if player has reached that position
			rp = *curr_point;
			if (gfc_vector2d_distance_between_less_than(player->position, rp, 8.0f))
				recall_manager.pathToPlayer_index--; // move to next point in list

			// if reached last position (note: account for max_positions)
			if ((recall_manager.pathToPlayer->count > recall_manager.max_positions && recall_manager.pathToPlayer_index < recall_manager.pathToPlayer->count - recall_manager.max_positions) 
				|| (recall_manager.pathToPlayer->count <= recall_manager.max_positions && recall_manager.pathToPlayer_index < 0))
				recall_manager.state = RECALL_FINISH;
			else {
				// calculate next move direction
				gfc_vector2d_sub(recall_manager.recall_dir, rp, player->position);
				gfc_vector2d_set_magnitude(&recall_manager.recall_dir, recall_manager.rewind_velocity_mag);
				recall_manager.state = RECALL_REWIND;
			}
			break;

		case RECALL_FINISH:
			player_recall_reset(player, p_data, time);
			//slog("finished");
			break;
	}
}

GFC_Vector2D* create_recall_pos(GFC_Vector2D pos) {
	GFC_Vector2D* rp;

	rp = gfc_allocate_array(sizeof(GFC_Vector2D), 1);
	if (!rp) return NULL;

	*rp = gfc_vector2d(pos.x, pos.y);

	return rp;
}

/*
float calc_total_recall_distance(GFC_Vector2D initial) {
	RecallPoint* rpf, * rpi;
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
	//slog("%f", dist);
	rpi = (RecallPoint*) gfc_list_nth(recall_manager.pathToPlayer, i);
	dist += gfc_vector2d_magnitude_between(initial, rpi->point);
	return dist;
}
*/

void player_recall_reset(void* p, void* data, float time) {
	PlayerData* p_data;
	Entity* player;

	player = (Entity*)p;
	p_data = (PlayerData*)data;
	if (!player || !p_data) return;

	gfc_list_foreach(recall_manager.pathToPlayer, free);
	gfc_list_clear(recall_manager.pathToPlayer);

	recall_manager.next_recording_time = time + 1.0f;
	recall_manager.next_recall = time + recall_manager.cooldown_time;

	player->grav_flag = 1;
	player->plat_flag = 1;
	p_data->state = PLAYER_IDLE;

	recall_manager.state = RECALL_NONE;
}

void trim_pathToPlayer(GFC_List* list) {
	GFC_Vector2D* rp;

	if (!list) return;

	rp = gfc_list_nth(list, 0);
	gfc_list_delete_nth(list, 0);
	free(rp);
}

// TODO: ADD COLLISION DETECTION
void player_bash(void* p, void* data) {
	PlayerData* p_data;
	Entity* player;
	GFC_Vector2D coll;
	GFC_Edge2D p_edge;

	player = (Entity*)p;
	p_data = (PlayerData*)data;
	if (!player || !p_data) return;

	switch (bash_manager.state) {
		case BASH_START:
			player->grav_flag = 0;
			player->plat_flag = 0;
			gfc_vector2d_clear(player->velocity);

			bash_manager.bash_velocity_mag = 15.0f;
			bash_manager.bashing_stop_time = CURRENT_TIME + bash_manager.bashing_duration;
			bash_manager.state = BASH_WAIT;

			break;
		case BASH_WAIT:
			if (gfc_input_command_held("movedown")) {
				bash_manager.bash_dir = BASH_DOWN;
				p_data->moveTypeY = PMOVE_FALLING;
			}
			else if (gfc_input_command_held("moveright")) {
				bash_manager.bash_dir = BASH_RIGHT;
				p_data->moveTypeX = PMOVE_RIGHT;
			}
			else if (gfc_input_command_held("moveleft")) {
				bash_manager.bash_dir = BASH_LEFT;
				p_data->moveTypeX = PMOVE_LEFT;
			}
			else { // if holding up or no input, always hold up
				bash_manager.bash_dir = BASH_UP;
				p_data->moveTypeY = PMOVE_RISING;
			}

			if (CURRENT_TIME > bash_manager.bashing_stop_time || gfc_input_command_released("bash")) {
				bash_manager.state = BASH_MOVE;

				switch (bash_manager.bash_dir) {
					case BASH_DOWN:
						bash_manager.bash_vel = gfc_vector2d(0, 1);
						break;
					case BASH_RIGHT:
						bash_manager.bash_vel = gfc_vector2d(1, 0);
						break;
					case BASH_LEFT:
						bash_manager.bash_vel = gfc_vector2d(-1, 0);
						break;
					default: // BASH_UP
						bash_manager.bash_vel = gfc_vector2d(0, -1);
				}
				gfc_vector2d_set_magnitude(&bash_manager.bash_vel, bash_manager.bash_velocity_mag);

				// collision detection set-up
				gfc_vector2d_copy(player->velocity, bash_manager.bash_vel); // FOR COLLISION CHECKING PURPOSES, DO NOT USE FOR BASH MOVEMENT
				bash_manager.wall_type = bash_manager.bash_dir == BASH_RIGHT ? WALL_LEFT : WALL_RIGHT;
			}

			break;

		case BASH_MOVE:
			// if collision detection, adjust move speed
			if ((bash_manager.bash_dir == BASH_RIGHT || bash_manager.bash_dir == BASH_LEFT) ||
				(entity_keep_in_bounds(player, bash_manager.wall_type) || wall_collision(player, bash_manager.wall_type))
				) {
				bash_manager.state = BASH_COLLIDED_MOVE;
				bash_manager.collided = 1;

				break;
			}
			else if (bash_manager.bash_dir == BASH_UP && ceiling_collision(player)){
				bash_manager.state = BASH_COLLIDED_MOVE;
				bash_manager.collided = 1;
				break;
			}
			else if (bash_manager.bash_dir == BASH_DOWN && ground_collision(player)) {
				bash_manager.state = BASH_COLLIDED_MOVE;
				bash_manager.collided = 1;

				bash_manager.collided_ground = get_colliding_ground(player);
				coll = gfc_vector2d(player->position.x, bash_manager.collided_ground.y);
				p_edge = get_edge_from_rect(player->boundbox.s.r, 0);

				bash_manager.bash_velocity_mag = coll.y - p_edge.y1;
				bash_manager.bash_vel = gfc_vector2d(0, 1);
				break;
			}
			
			gfc_vector2d_add(player->position, player->position, bash_manager.bash_vel);
			
			// slowdown bash speed
			bash_manager.bash_velocity_mag -= bash_manager.bash_decel_mag;
			gfc_vector2d_set_magnitude(&bash_manager.bash_vel, bash_manager.bash_velocity_mag);

			// stop bashing after below certain speed
			if (bash_manager.bash_velocity_mag <= 5.0f)
				bash_manager.state = BASH_STOP;

			break;
		case BASH_COLLIDED_MOVE:
			switch (bash_manager.bash_dir) {
				case BASH_DOWN:
					gfc_vector2d_set_magnitude(&bash_manager.bash_vel, 0.5f* bash_manager.bash_velocity_mag);
					//0.4f*(coll.y - p_edge.y1)
					break;
				case BASH_RIGHT:
					bash_manager.bash_vel = gfc_vector2d(1, 0);
					break;
				case BASH_LEFT:
					bash_manager.bash_vel = gfc_vector2d(-1, 0);
					break;
				default: // BASH_UP
					bash_manager.bash_vel = gfc_vector2d(0, -1);
			}

			gfc_vector2d_add(player->position, player->position, bash_manager.bash_vel);

			//bash_manager.state = BASH_STOP;

			break;

		case BASH_STOP:
			player->grav_flag = 1;
			player->plat_flag = 1;
			p_data->state = PLAYER_MOVING;

			// carry over leftover velocity if no collision
			if (!bash_manager.collided) {
				if (bash_manager.bash_dir != BASH_RIGHT)
					gfc_vector2d_negate(bash_manager.bash_vel, bash_manager.bash_vel);
				gfc_vector2d_copy(player->velocity, bash_manager.bash_vel);
			}
			else {
				slog("collided");
				gfc_vector2d_clear(player->velocity);
			}

			bash_manager.state = BASH_NONE;
			break;
	}
}
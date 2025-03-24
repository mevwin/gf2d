#include "simple_logger.h"
#include "world.h"
#include "player.h"
#include "collisions.h"

typedef struct RecallManager_S {
	// timing
	int					cooldown_frames;		// cooldown duration after recalling
	int					cooldown_counter;		// counts the frames until it matches cooldown_frames
	int					frames_to_record;		// time to wait for recording next player position
	float				then_track;				// tracker variable for position tracking time-deltas
	float				then_cooldown;			// tracker variable for cooldown time time-deltas

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
}RecallManager;

typedef struct BashManager_S {
	BashState			state;					// current state of bashing process
	Entity*				bashed_obj;
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

void player_move_check_input(Entity* player);

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
	recall_manager.cooldown_frames = 360;
	recall_manager.frames_to_record = 12;
	recall_manager.cooldown_counter = 359;
	recall_manager.state = RECALL_NONE;
	recall_manager.max_positions = 10;
	recall_manager.pathToPlayer = gfc_list_new_size(recall_manager.max_positions);
	
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

void player_move(void* p) {
	PlayerData* p_data;
	Entity* player;
	Uint8 wall_type;

	player = (Entity*) p;
	if (!player) return;

	p_data = player->data;
	if (!p_data || p_data->state == PLAYER_RECALL || p_data->state == PLAYER_BASH || !p_data->canMove) return;

	player_move_check_input(player);

	// 0 == left wall, 1 == right wall
	wall_type = player->dir.x == 0 ? 0 : 1;

	/* big ass state check to actually apply the movement */
	switch (p_data->state) {
		case PLAYER_MOVING:
			if ((wall_collision(player, wall_type) && !p_data->wall_jump) || entity_keep_in_bounds(player, wall_type)) {
				player->velocity.x = 0;
				p_data->state = PLAYER_IDLE;
				return;
			}

			if (player->velocity.x > player->max_velocity.x)
				player->velocity.x -= p_data->friction.z;

			player->position.x += p_data->moveTypeX == PMOVE_RIGHT ?
									player->velocity.x : -player->velocity.x;

			break;

		case PLAYER_SLOWDOWN:
			if (wall_collision(player, wall_type) || entity_keep_in_bounds(player, wall_type)) {
				player->velocity.x = 0;
				p_data->state = PLAYER_IDLE;
				return;
			}

			// sliding effect
			player->position.x += p_data->moveTypeX == PMOVE_RIGHT ?
				player->velocity.x : -player->velocity.x;

			// if on ground, apply friction
			if ((ground_collision(player) || platform_collision(player)) && player->velocity.x > 0) {
				player->velocity.x -= p_data->turnaround ? p_data->friction.x : p_data->friction.y;

				if (player->velocity.x < 0) {
					player->velocity.x = 0;
					p_data->moveTypeX = PMOVE_NONE_X;
					p_data->state = PLAYER_IDLE;
				}
			}
			else { // if in air and turning around, slow down
				player->velocity.x -= p_data->friction.x;

				if (player->velocity.x < 0) {
					p_data->moveTypeX = p_data->moveTypeX != PMOVE_RIGHT ? PMOVE_RIGHT : PMOVE_LEFT;
					p_data->turnaround = 1;
					player->velocity.x = 0;
				}
			}

			break;

		case PLAYER_DODGE:
			if (entity_keep_in_bounds(player, wall_type) || wall_collision(player, wall_type)) {
				player->velocity.x = 0;
				p_data->state = PLAYER_IDLE;
				return;
			}

			player->position.x += p_data->moveTypeX == PMOVE_RIGHT ?
				player->velocity.x : -player->velocity.x;

			player->velocity.x -= p_data->dodge_vel_reduc;

			// dodge jump momentum carrying
			if (!ground_collision(player) && !platform_collision(player) && gfc_input_command_pressed("jump"))
				p_data->state = PLAYER_MOVING;
			else if (player->velocity.x < 0.4f * p_data->dodge_vel.y) {
				player->velocity.x = 0.4f * p_data->dodge_vel.y;
				p_data->state = PLAYER_SLOWDOWN;
			}

			break;
	}
}

void player_move_check_input(Entity* player) {
	PlayerData* p_data;
	Uint8 wall_type;

	p_data = player->data;
	if (!p_data) return;

	/* BASE HORIZONTAL MOVEMENT */
	// to help maintain momentum in the air
	if (!gfc_input_command_down("moveright") && !gfc_input_command_down("moveleft")
		&& player->velocity.x > 0 && !ground_collision(player) && !platform_collision(player) && p_data->state != PLAYER_DODGE)
		p_data->state = PLAYER_MOVING;

	/*
	if ((gfc_input_command_released("moveleft") || gfc_input_command_released("moveright"))
		&& player->velocity.x > 0 && p_data->state != PLAYER_DODGE)
		p_data->state = PLAYER_SLOWDOWN;
	*/

	// input checks
	if ((gfc_input_command_down("moveleft") || gfc_input_command_pressed("moveleft"))
		&& !gfc_input_command_pressed("moveright") && p_data->state != PLAYER_DODGE && !p_data->wall_jump) {
		p_data->state = PLAYER_MOVING;

		if (p_data->moveTypeX == PMOVE_RIGHT && player->velocity.x > 0) {
			p_data->state = PLAYER_SLOWDOWN;
			if (ground_collision(player) || platform_collision(player))
				p_data->turnaround = 1;
		}
		else
			p_data->moveTypeX = PMOVE_LEFT;

		if (player->velocity.x <= player->max_velocity.x) // ground
			player->velocity.x += player->accel.x;
		else if (p_data->turnaround && player->velocity.x <= player->max_velocity.x) // air
			player->velocity.x += player->accel.x * 2.0f;
	}
	else if ((gfc_input_command_down("moveright") || gfc_input_command_pressed("moveright"))
		&& !gfc_input_command_pressed("moveleft") && p_data->state != PLAYER_DODGE && !p_data->wall_jump) {
		p_data->state = PLAYER_MOVING;

		if (p_data->moveTypeX == PMOVE_LEFT && player->velocity.x > 0) {
			p_data->state = PLAYER_SLOWDOWN;
			if (ground_collision(player) || platform_collision(player))
				p_data->turnaround = 1;
		}
		else
			p_data->moveTypeX = PMOVE_RIGHT;

		if (player->velocity.x <= player->max_velocity.x) // ground
			player->velocity.x += player->accel.x;
		else if (p_data->turnaround && player->velocity.x <= player->max_velocity.x) // air
			player->velocity.x += player->accel.x * 2.0f;
	}

	/* DODGE */
	if (p_data->canDodge && gfc_input_command_pressed("dodge") && p_data->state != PLAYER_DODGE && p_data->dodge_charges > 0) {
		if (gfc_input_command_pressed("moveright") || player->dir.x == 0)
			p_data->moveTypeX = PMOVE_RIGHT;
		else if (gfc_input_command_pressed("moveleft") || player->dir.x == 1)
			p_data->moveTypeX = PMOVE_LEFT;

		p_data->state = PLAYER_DODGE;
		p_data->moveTypeY = PMOVE_NONE_Y;

		player->velocity.x = p_data->dodge_vel.x;
		if (!ground_collision(player) && !platform_collision(player)) {
			p_data->dodge_charges--;
			player->velocity.x = p_data->dodge_vel.y;
		}
		player->velocity.y = 0;
	}

	/* RECALL */
	if (p_data->canRecall && gfc_input_command_pressed("recall")) {
		if (recall_manager.cooldown_counter > recall_manager.cooldown_frames) {
			recall_manager.state = RECALL_START;
			p_data->state = PLAYER_RECALL;
			return;
		}
		else
			slog("waiting");
	}

	/* BASH */
	if (p_data->canBash && !ground_collision(player) && p_data->state != PLAYER_BASH && gfc_input_command_pressed("bash")) {
		//if (CURRENT_TIME >= bash_manager.next_bash) { // if next to a bash-able object
		bash_manager.state = BASH_START;
		p_data->state = PLAYER_BASH;
		return;
		//}
		//else
			//slog("waiting");
	}

	// 0 == left wall, 1 == right wall
	wall_type = player->dir.x == 0 ? 0 : 1;

	/* BASE VERTICAL MOVEMENT (NOTE: VERTICAL MEANS NEGATIVE Y)*/
	/* JUMP */
	if (gfc_input_command_pressed("jump")) {
		if (p_data->canWallJump && wall_collision(player, wall_type) && !p_data->wall_jump && p_data->moveTypeY != PMOVE_NONE_Y) { // wall_jump
			p_data->wall_jump = 1;
			player->velocity.y = p_data->jump_speed;
			player->velocity.x = player->max_velocity.x;
			p_data->moveTypeX = wall_type ? PMOVE_RIGHT : PMOVE_LEFT; // if wall jumping from right, go left (and vice versa)
			p_data->moveTypeY = PMOVE_RISING;

			if (!player->plat_flag)
				player->plat_flag = 1;
		}
		else if (p_data->jump_count < p_data->max_jumps) {
			if (!p_data->canDoubleJump && p_data->jump_count) return;

			player->velocity.y = p_data->jump_speed;

			p_data->jump_count++;
			p_data->moveTypeY = PMOVE_RISING;

			if (!player->plat_flag)
				player->plat_flag = 1;
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

	/*
	// FLOAT ABILITY
	if (p_data->moveTypeY == PMOVE_FALLING && gfc_input_command_held("moveup")) {
		player->velocity.y = -0.5f;
	}
	*/

	/* MOVE THROUGH PLATFORM */
	if (p_data->moveTypeY == PMOVE_FASTFALLING ||
		(platform_collision(player) && (gfc_input_command_pressed("movedown") || gfc_input_command_down("movedown")))
		) {
		player->plat_flag = 0;
	}
}

void track_player(void* p, void* data) {
	PlayerData* p_data;
	Entity* player;
	float time;

	player = (Entity*)p;
	p_data = (PlayerData*)data;
	if (!player || !p_data || recall_manager.state != RECALL_NONE || !p_data->canMove) return;
	
	time = CURRENT_TIME;

	// increment cooldown counter frames
	if (time - recall_manager.then_cooldown > FRAME_DUR && recall_manager.cooldown_counter <= recall_manager.cooldown_frames) {
		recall_manager.cooldown_counter++;
		recall_manager.then_cooldown = time;
	}

	if (time - recall_manager.then_track > (FRAME_DUR * recall_manager.frames_to_record)) {
		if (recall_manager.pathToPlayer->count > recall_manager.max_positions) 
			trim_pathToPlayer(recall_manager.pathToPlayer);

		gfc_list_append(recall_manager.pathToPlayer, create_recall_pos(player->position));
		recall_manager.then_track = time;
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
	if (!player || !p_data  || !p_data->canMove) return;
	
	time = CURRENT_TIME;
	if (!recall_manager.pathToPlayer->count) { 
		recall_manager.state = RECALL_NONE;
		recall_manager.cooldown_counter = 0;
		slog("no recorded positions");
		return; 
	}

	switch (recall_manager.state) {
		case RECALL_START: // prepare player and recall_manager for recall
			//slog("recall stared: %i positions", recall_manager.pathToPlayer->count);
			recall_manager.pathToPlayer_index = (int)recall_manager.pathToPlayer->count - 1;
			recall_manager.cooldown_counter = 0;

			// stop current player movement
			gfc_vector2d_clear(player->velocity);
			player->grav_flag = 0;
			player->plat_flag = 0;
			player->canBeDamaged = 0;

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
			if (gfc_input_command_pressed("jump")) {
				recall_manager.state = RECALL_FINISH;
				break;
			}

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

	recall_manager.then_cooldown = time;
	recall_manager.then_track = time + 1.5f;
	player->grav_flag = 1;
	player->plat_flag = 1;
	player->canBeDamaged = 1;
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

// TODO: 
// ADD COLLISION DETECTION
// USE FRAME-BASED TIMING INSTEAD OF HARD TIME CHECKS WITH SDL
void player_bash(void* p, void* data) {
	PlayerData* p_data;
	Entity* player;
	GFC_Vector2D coll;
	GFC_Edge2D p_edge;
	GFC_Rect hitbox;

	player = (Entity*)p;
	p_data = (PlayerData*)data;
	if (!player || !p_data || !p_data->canMove) return;

	switch (bash_manager.state) {
		case BASH_START:
			hitbox = gfc_rect(player->position.x - player->hurtbox.s.r.w * 0.8f,
							  player->position.y - player->hurtbox.s.r.h * 0.8f,
							  player->hurtbox.s.r.w * 1.6f, 
							  player->hurtbox.s.r.h * 1.6f);

			bash_manager.bashed_obj = find_nearest_entity(player, &hitbox, SEARCH_BASH);
			if (!bash_manager.bashed_obj) {
				bash_manager.state = BASH_NONE;
				p_data->state = PLAYER_MOVING;
				slog("no entity found");
				return;
			}

			bash_manager.bashed_obj->bash_flag = 1;
			bash_manager.state = BASH_START;

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
			/*
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
			*/
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

			bash_manager.bashed_obj->bash_flag = 0;
			bash_manager.state = BASH_NONE;
			break;
	}
}
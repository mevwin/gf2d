#include "simple_logger.h"
#include "gf2d_draw.h"
#include "world.h"
#include "collisions.h"
#include "player.h"
#include "enemy.h"

EnemyData* enemy_data_init(SJson* data, EnemyType type);

void enemy_think(Entity* self);
void enemy_update(Entity* self);
void enemy_gravity(Entity* self);
void enemy_free(Entity* self);

void enemy_die(Entity* self, EnemyData* e_data);
void enemy_move(Entity* self);
void enemy_attack(Entity* self);

void enemy_spawn(int type, GFC_Vector2D position, SJson* sprouter_spawns) {
	SJson* file, *enemy_type, *init_data;
	EnemyData* e_data;
	GFC_List* enemy_list;
	Entity* enemy;
	int i;

	enemy = entity_new();
	if (!enemy) {
		slog("failed to initialize enemy");
		return;
	}

	file = sj_load("def/enemy.def");
	if (!file) {
		slog("couldn't get enemy.def");
		entity_free(enemy);
		return;
	}

	enemy_type = sj_array_get_nth(sj_object_get_value(file, "enemy_list"), type);
	if (!enemy_type) {
		slog("invalid index");
		entity_free(enemy);
		return;
	}
	init_data = sj_object_get_value(enemy_type, "entity_data");

	enemy->type = ENEMY;
	//gfc_line_cpy(enemy->name, "Enemy");

	gfc_vector2d_copy(enemy->position, position);
	enemy->velocity = gfc_vector2d(0, 0);

	sj_object_get_vector2d(init_data, "max_velocity", &enemy->max_velocity);
	sj_object_get_vector2d(init_data, "accel", &enemy->max_velocity);
	enemy->sprite = gf2d_sprite_load_image(sj_object_get_string(init_data, "sprite"));
	enemy->frame = 0;
	
	enemy->think = enemy_think;
	enemy->update = enemy_update;
	enemy->grav = enemy_gravity;
	enemy->free = enemy_free;

	enemy->scale = gfc_vector2d(1, 1);
	enemy->dir = gfc_vector2d(0, 0);

	enemy->data = enemy_data_init(sj_object_get_value(enemy_type, "enemy_data"), type);
	e_data = enemy->data;
	if (e_data->type == SPROUTER) {
		enemy->grav_flag = 0;

		if (sprouter_spawns) {
			e_data->sprouter_spawns = gfc_allocate_array(sizeof(GFC_Vector2D), sprouter_spawns->v.array->count);
			for (i = 0; i < sprouter_spawns->v.array->count; i++) {
				sj_object_get_vector2d(sj_array_nth(sprouter_spawns, i), "position", &e_data->sprouter_spawns[i]);
			}
			e_data->sprouter_spawns_count = sprouter_spawns->v.array->count;
		}

		e_data->sprouter_spawn_index = 0;
		e_data->sprouter_idle_counter = 0;
		e_data->then = CURRENT_TIME;
	}
	else if (e_data->type == CHASER)
		enemy->grav_flag = 0;
	else
		enemy->grav_flag = 1;

	update_hurtbox(enemy);
	update_boundbox(enemy);

	enemy_list = get_enemy_list();
	gfc_list_append(enemy_list, enemy);
}

EnemyData* enemy_data_init(SJson* data, EnemyType type) {
	EnemyData* e_data;

	if (!data) {
		slog("no json file");
		return NULL;
	}

	e_data = gfc_allocate_array(sizeof(EnemyData), 1);
	if (!e_data) {
		slog("failed to allocate space fo e_data");
		return NULL;
	}

	e_data->type = type;
	if (e_data->type == SPROUTER) {
		sj_object_get_uint32(data, "sprouter_idle_frames", &e_data->sprouter_idle_frames);
	}

	sj_object_get_float(data, "maxHealth", &e_data->maxHealth);
	e_data->currHealth = e_data->maxHealth;

	return e_data;
}

void enemy_think(Entity* self) {
	EnemyData* e_data;

	e_data = self->data;
	if (!e_data) return;

	//if (self->velocity.y == 0)
		//self->velocity.y = 9.0f;

	enemy_move(self);
	enemy_attack(self);
}

void enemy_update(Entity* self) {
	EnemyData* e_data;

	e_data = self->data;
	if (!e_data) return;

	if (e_data->currHealth <= 0.0f)
		enemy_die(self, e_data);
}

void enemy_gravity(Entity* self) {
	EnemyData* e_data;

	e_data = self->data;
	if (!e_data) return;

	if ((ground_collision(self) || platform_collision(self)) && self->velocity.y == 0) { // grounded
		//  uhhhh....
	}
	else if (ground_collision(self) || platform_collision(self)) { // landing
		self->velocity.y = 0;
	}
	else {
		self->position.y -= self->velocity.y;
		self->velocity.y -= GRAVITY;
	}
}

void enemy_free(Entity* self) {
	GFC_List* enemy_list;
	EnemyData* e_data;

	enemy_list = get_enemy_list();
	gfc_list_delete_data(enemy_list, self);

	gf2d_sprite_free(self->sprite);

	if (self->data) { 
		e_data = self->data;
		if (e_data->type == SPROUTER && e_data->sprouter_spawns)
			free(e_data->sprouter_spawns);

		free(e_data);
	}
}

void enemy_move(Entity* self) {
	EnemyData* e_data;
	GFC_Vector2D player_pos, chaser_dir;
	float time;

	e_data = self->data;
	if (!e_data) return;

	time = CURRENT_TIME;
	switch (e_data->type) {
		case SPROUTER:
			if (!e_data->sprouter_spawns_count) return;

			if (time - e_data->then > FRAME_DUR && e_data->sprouter_idle_counter < e_data->sprouter_idle_frames) {
				e_data->sprouter_idle_counter++;
				e_data->then = time;
			}

			//slog("%i", e_data->sprouter_idle_counter);

			if (e_data->sprouter_idle_counter == e_data->sprouter_idle_frames) {
				e_data->sprouter_idle_counter = 0;

				
				if (e_data->sprouter_spawn_index == e_data->sprouter_spawns_count)
					e_data->sprouter_spawn_index = 0;

				gfc_vector2d_copy(self->position, e_data->sprouter_spawns[e_data->sprouter_spawn_index]);
				e_data->sprouter_spawn_index++;
			}
			
			break;

		case CHASER:
			gfc_vector2d_copy(player_pos, get_player_pos());

			gfc_vector2d_sub(chaser_dir, player_pos, self->position);
			gfc_vector2d_set_magnitude(&chaser_dir, gfc_vector2d_magnitude(self->max_velocity));
			gfc_vector2d_add(self->position, self->position, chaser_dir);

			break;

		default:
			;
	}
}

void enemy_attack(Entity* self) {
	EnemyData* e_data;
	PlayerData* p_data;
	Entity* player;
	GFC_Rect sprouter_hbox;
	float time;

	e_data = self->data;
	if (!e_data) return;

	player = (Entity*) get_player();
	if (!player) return;

	p_data = (PlayerData*)player->data;
	if (!p_data) return;

	time = CURRENT_TIME;

	switch (e_data->type) {
		case SPROUTER:
			sprouter_hbox = gfc_rect(self->position.x - 125.0f,
									self->position.y - 50.0f,
									250.0f,
									100.0f);

			gf2d_draw_rect_filled(sprouter_hbox, gfc_color8(0, 120, 120, 120));

			if (player->canBeDamaged && gfc_rect_overlap(sprouter_hbox, player->hurtbox.s.r)) {
				//slog("hurting player");
				p_data->currHealth -= 0.1f;
			}

			break;

		case CHASER:
			if (gfc_rect_overlap(self->hurtbox.s.r, player->hurtbox.s.r)) {
				//slog("hurting player");
				p_data->currHealth -= 0.05f;
			}
			//gfc_vector2d_copy(player_pos, get_player_pos());

			//gfc_vector2d_sub(chaser_dir, player_pos, self->position);
			//gfc_vector2d_set_magnitude(&chaser_dir, gfc_vector2d_magnitude(self->max_velocity));
			//gfc_vector2d_add(self->position, self->position, chaser_dir);

			break;

		default:
			;
	}
}

void enemy_die(Entity* self, EnemyData* e_data) {
	if (!self || !e_data) return;

	
	entity_free(self);
}
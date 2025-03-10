#include "simple_logger.h"
#include "world.h"
#include "collisions.h"
#include "enemy.h"

EnemyData* enemy_data_init(SJson* data, EnemyType type);

void enemy_think(Entity* self);
void enemy_update(Entity* self);
void enemy_gravity(Entity* self);
void enemy_free(Entity* self);

void enemy_die(Entity* self, EnemyData* e_data);
void enemy_move(Entity* self);

void enemy_spawn(int type, GFC_Vector2D position) {
	SJson* file, *enemy_type, *init_data;
	GFC_List* enemy_list;
	Entity* enemy;

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
	enemy->grav_flag = 1;
	enemy->free = enemy_free;

	enemy->scale = gfc_vector2d(1, 1);
	enemy->dir = gfc_vector2d(0, 0);

	enemy->data = enemy_data_init(sj_object_get_value(enemy_type, "enemy_data"), type);

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

	enemy_list = get_enemy_list();
	gfc_list_delete_data(enemy_list, self);

	gf2d_sprite_free(self->sprite);

	if (self->data) free(self->data);
}

void enemy_move(Entity* self) {
	EnemyData* e_data;

	e_data = self->data;
	if (!e_data) return;

}

void enemy_die(Entity* self, EnemyData* e_data) {
	if (!self || !e_data) return;

	
	entity_free(self);
}
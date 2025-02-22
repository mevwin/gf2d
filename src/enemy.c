#include "simple_logger.h"
#include "world.h"
#include "collisions.h"
#include "enemy.h"

void enemy_think(Entity* self);
void enemy_update(Entity* self);
void enemy_gravity(Entity* self);
void enemy_free(Entity* self);

void enemy_move(Entity* self);

void enemy_spawn(EnemyType type, GFC_Vector2D position) {
	Entity* enemy;

	enemy = entity_new();
	if (!enemy) {
		slog("failed to initialize enemy");
		return NULL;
	}

	enemy->type = ENEMY;
	gfc_line_cpy(enemy->name, "Enemy");

	gfc_vector2d_copy(enemy->position, position);
	enemy->velocity = gfc_vector2d(0, 0);
	enemy->max_velocity = gfc_vector2d(5.0f, 5.0f);
	enemy->accel = gfc_vector2d(0.1f, 0.2f);

	enemy->sprite = gf2d_sprite_load_image("sprites/coke.png");
	enemy->frame = 0;
	
	enemy->think = enemy_think;
	enemy->update = enemy_update;
	enemy->grav = enemy_gravity;
	enemy->grav_flag = 1;
	enemy->free = enemy_free;

	enemy->scale = gfc_vector2d(1, 1);
	enemy->dir = gfc_vector2d(0, 0);

	update_hurtbox(enemy);
	update_boundbox(enemy);

	world_append_enemy(enemy);
}

void enemy_think(Entity* self) {
	//if (self->velocity.y == 0)
		//self->velocity.y = 9.0f;

}

void enemy_update(Entity* self) {

}

void enemy_gravity(Entity* self) {
	GFC_Edge2D bottom;

	bottom = get_edge_from_rect(self->boundbox.s.r, 0);

	if (ground_collision(self) && self->velocity.y == 0) { // grounded
		//  uhhhh....
	}
	else if (ground_collision(self)) { // landing
		self->velocity.y = 0;
	}
	else {
		self->position.y -= self->velocity.y;
		self->velocity.y -= GRAVITY;
	}
}

void enemy_free(Entity* self) {
	gf2d_sprite_delete(self->sprite);

	if (self->data) free(self->data);
}

void enemy_move(Entity* self) {
	//EnemyData* e_data;

	

}
#include "simple_logger.h"
#include "gf2d_draw.h"
#include "world.h"
#include "collisions.h"
#include "player.h"
#include "camera.h"

typedef struct Camera_S {
	Uint8			active;
	//float			move_speed_mag;
	//GFC_Vector2D	direction;
	GFC_Vector2D	position;		// top right corner
	GFC_Vector2D	move_speed;
	GFC_Rect*		move_space;		// left: 0, right: 1
}Camera;

static Camera camera = { 0 };

void camera_close();
void camera_apply_bounds(GFC_Vector2D level_size);

void camera_init() {
	camera.active = 1;
	camera.position = gfc_vector2d(0, 0);
	camera.move_speed = gfc_vector2d(0, 0);
	//camera.direction = gfc_vector2d(1, 1);
	camera.move_space = gfc_allocate_array(sizeof(GFC_Rect), 4);
	camera.move_space[0] = gfc_rect(0, 0, RES.w * 0.35f, RES.h);
	camera.move_space[1] = gfc_rect(RES.w - (RES.w * 0.35f), 0, RES.w * 0.35f, RES.h);

	atexit(camera_close);
}

void camera_update(void* p, GFC_Vector2D level_size) {
	Entity* player;
	PlayerData* p_data;

	player = (Entity*)p;
	if (!p || !camera.active) return;

	p_data = (PlayerData*) player->data;

	//gf2d_draw_rect(camera.move_space[0], GFC_COLOR_RED);
	//gf2d_draw_rect(camera.move_space[1], GFC_COLOR_RED);

	if ((gfc_rect_overlap(player->boundbox.s.r, camera.move_space[0]) && p_data->moveTypeX == PMOVE_LEFT)
		|| (gfc_rect_overlap(player->boundbox.s.r, camera.move_space[1]) && p_data->moveTypeX == PMOVE_RIGHT))
		{
		//slog("moving camera");

		gfc_vector2d_copy(camera.move_speed, player->velocity);
		if (player->dir.x == 1)
			camera.move_speed.x = -player->velocity.x;
		//camera.move_speed.y = -player->velocity.y;
		camera.move_speed.y = 0;

		camera.move_speed.x *= 2.0f;

		gfc_vector2d_add(camera.position, camera.position, camera.move_speed);

		camera_apply_bounds(level_size);

		// move the level
		level_camera_update(camera.move_speed);
		//player->velocity.x = 0;
		//gfc_vector2d_clear(player->velocity);
	}

	if (gfc_input_command_pressed("display")) {
		slog("camera left position: (%f, %f)", camera.position.x, camera.position.y);
		slog("camera right position: (%f, %f)", camera.position.x + RES.w, camera.position.y);
		slog("player position: (%f, %f)", player->position.x, player->position.y);
	}
}

void camera_apply_bounds(GFC_Vector2D level_size) {
	// check for each edge with the level size
	if (camera.position.x + camera.move_speed.x < 1.0f) {
		camera.position.x = 0;
		camera.move_speed.x = 0;
	}
	if (camera.position.x + RES.w + camera.move_speed.x > level_size.x - 1.0f) {
		camera.position.x = level_size.x - RES.w;
		camera.move_speed.x = 0;
	}
	if (camera.position.y + camera.move_speed.y < 1.0f) {
		camera.position.y = 0;
	}
	if (camera.position.y + RES.h + camera.move_speed.y > level_size.y - 1.0f) {
		camera.position.y = level_size.y - RES.h;
	}
}

void camera_close() {
	memset(&camera, 0, sizeof(Camera));
}
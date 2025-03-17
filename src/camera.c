#include "simple_logger.h"
#include "gf2d_draw.h"
#include "world.h"
#include "collisions.h"
#include "player.h"
#include "camera.h"

typedef struct Camera_S {
	Uint8			active;
	//float			move_speed_mag;
	GFC_Vector2D	direction;
	GFC_Vector2D	move_speed;
	GFC_Rect		screen_space;
}Camera;

static Camera camera = { 0 };

void camera_close();

void camera_init() {
	camera.active = 1;
	camera.move_speed = gfc_vector2d(0, 0);
	camera.direction = gfc_vector2d(1, 1);
	camera.screen_space = gfc_rect(150, 105, 900, 490);

	atexit(camera_close);
}

void camera_update(void* p) {
	Entity* player;

	player = (Entity*)p;
	if (!p || !camera.active) return;

	gf2d_draw_rect(camera.screen_space, GFC_COLOR_RED);

	if (!gfc_rect_overlap(player->boundbox.s.r, camera.screen_space)) {
		//slog("moving camera");

		gfc_vector2d_negate(camera.move_speed, player->velocity);
		if (player->dir.x == 0)
			camera.move_speed.x = -player->velocity.x;
		else // going left 
			camera.move_speed.x = player->velocity.x;


		// move the level
		level_camera_update(camera.move_speed);


		//init_camera_movement(player);
	}
}

void camera_close() {
	memset(&camera, 0, sizeof(Camera));
}
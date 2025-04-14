#include "simple_logger.h"
#include "gfc_config.h"
#include "gf2d_sprite.h"
#include "mouse.h"
#include "world.h"

typedef struct Mouse_S {
	Sprite*			sprite;
	Uint32			mask;
	GFC_Color		color;
	int				pos_x;
	int				pos_y;
	float			frame;

	Uint8			pressed;
	Uint8			released;
}Mouse;

static Mouse mouse = { 0 };

void mouse_close();

void mouse_init() {
	SJson* config;
	int w, h, l, k;

	config = sj_load("config/mouse.cfg");
	if (!config) {
		slog("mouse.cfg not found");
		return NULL;
	}

	sj_object_get_int(config, "frameWidth", &w);
	sj_object_get_int(config, "frameHeight", &h);
	sj_object_get_int(config, "framesPerLine", &l);
	sj_object_get_int(config, "keepSurface", &k);
	mouse.sprite = gf2d_sprite_load_all(sj_object_get_string(config, "sprite"), w, h, l, k);
	mouse.color = sj_object_get_color(config, "color");
	mouse.frame = 0;
	mouse.mask = 0;

	mouse.pressed = 0;
	mouse.released = 0;

	sj_free(config);
	atexit(mouse_close);
}

void mouse_update_state() {
	mouse.frame += 0.1;
	if (mouse.frame >= 16.0)
		mouse.frame = 0;

	mouse.mask = SDL_GetMouseState(&mouse.pos_x, &mouse.pos_y);

	if (!mouse.mask) mouse.pressed = 0;
	//if (mouse.released) mouse.released = 0;
}

void draw_mouse() {
	gf2d_sprite_draw(
		mouse.sprite,
		gfc_vector2d(mouse.pos_x, mouse.pos_y),
		NULL,
		NULL,
		NULL,
		NULL,
		&mouse.color,
		(int) mouse.frame);
}

Uint8 mouse_button_pressed(MouseButton button) {
	if (button == mouse.mask && !mouse.pressed) {
		mouse.pressed = 1;
		mouse.released = 0;
		return 1;
	}
	else return 0;
}

Uint8 mouse_button_held(MouseButton button) {
	mouse_button_pressed(button);
	if (button == mouse.mask && mouse.pressed && !mouse.released) 
		return 1;
	else return 0;
}

//Uint8 mouse_button_released(MouseButton button) {
	//mouse_button_pressed(button);
	//return mouse.released;
//}

Uint8 mouse_in_rect(GFC_Rect rect) {
	GFC_Vector2D mp;

	mp = gfc_vector2d(mouse.pos_x, mouse.pos_y);
	return gfc_point_in_rect(mp, rect);
}

void mouse_close() {
	gf2d_sprite_delete(mouse.sprite);
	memset(&mouse, 0, sizeof(Mouse));
}
#ifndef __MOUSE_H__
#define __MOUSE_H__

#include "gfc_shape.h"

typedef enum MouseButton_E {
	MOUSE_LEFT_CLICK = 1,
	MOUSE_MIDDLE_CLICK,
	MOUSE_RIGHT_CLICK
}MouseButton;

void mouse_init();
void mouse_update_state();
void draw_mouse();
Uint8 mouse_button_pressed(MouseButton button);
Uint8 mouse_button_held(MouseButton button);
//Uint8 mouse_button_released(MouseButton button);
Uint8 mouse_in_rect(GFC_Rect rect);

#endif
#ifndef __LEVEL_EDITOR_H__
#define __LEVEL_EDITOR_H__

#include "gfc_vector.h"

typedef enum EditorState_E {
	EDITOR_ROOM_INIT,
	EDITOR_ROOM_LAYOUT
}EditorState;

void level_editor_init();
void level_editor_update();
void level_editor_close();


void level_editor_inc_room_count();
void level_editor_dec_room_count();

void level_editor_inc_room_layout_x();
void level_editor_dec_room_layout_x();

void level_editor_inc_room_layout_y();
void level_editor_dec_room_layout_y();

EditorState get_level_editor_state();
int get_level_editor_room_count();
GFC_Vector2D get_level_editor_room_layout();

void set_level_editor_state(EditorState state);
void predict_room_layout();

#endif
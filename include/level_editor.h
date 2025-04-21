#ifndef __LEVEL_EDITOR_H__
#define __LEVEL_EDITOR_H__

#include "gfc_vector.h"

typedef enum EditorState_E {
	EDITOR_SELECT_MODE,
	EDITOR_EXISTING_LEVELS,
	EDITOR_EDITING
}EditorState;

void level_editor_init();
void level_editor_update();
void level_editor_close();

//void level_editor_create_new_level();
//void level_editor_load_existing_level();

void initialize_level_previews();
void free_level_previews();
void level_editor_draw_level_previews();

void level_editor_inc_level_preview_pg();
void level_editor_dec_level_preview_pg();

EditorState get_level_editor_state();
int get_level_editor_room_count();
GFC_Vector2D get_level_editor_room_layout();

void set_level_editor_state(EditorState state);

#endif
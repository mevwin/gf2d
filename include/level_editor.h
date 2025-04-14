#ifndef __LEVEL_EDITOR_H__
#define __LEVEL_EDITOR_H__

typedef enum EditorState_E {
	EDITOR_ROOM_INIT
}EditorState;

void level_editor_init();
void level_editor_update();
void level_editor_close();

void level_editor_inc_room_count();
void level_editor_dec_room_count();

EditorState get_level_editor_state();
void set_level_editor_state(EditorState state);

#endif
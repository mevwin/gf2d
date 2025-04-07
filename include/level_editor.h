#ifndef __LEVEL_EDITOR_H__
#define __LEVEL_EDITOR_H__

typedef enum EditorState_E {
	EDITOR_START,
	EDITOR_LAYOUT,
	EDITOR_SAVE,
	EDITOR_CLOSE,
	EDITOR_ADD_ENTITY,
	EDITOR_ADD_ROOM,
	EDITOR_CHANGE_ROOM,
	EDITOR_REMOVE_ROOM,
	EDITOR_EDITING
}EditorState;

void level_editor_init();
void level_editor_update();
void level_editor_close();

#endif
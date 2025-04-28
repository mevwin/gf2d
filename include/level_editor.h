#ifndef __LEVEL_EDITOR_H__
#define __LEVEL_EDITOR_H__

#include "gfc_vector.h"
#include "entity.h"

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

/**
* @brief pre-load all entities in their default states
*/
void initialize_level_editor_all_entities();
void initialize_level_editor_entity_previews();
void change_level_editor_list_type(EntityType new);

void free_level_previews();

void level_editor_draw_level_previews();
void level_editor_draw_entity_preview_region();

void level_editor_inc_level_preview_pg();
void level_editor_dec_level_preview_pg();
void level_editor_next_list_type();
void level_editor_prev_list_type();
void level_editor_next_ent();
void level_editor_prev_ent();

EditorState get_level_editor_state();
int get_level_editor_room_count();
GFC_Vector2D get_level_editor_room_layout();
Uint8 get_level_editor_hud_toggle();
EntityType get_level_editor_list_type();
GFC_TextWord* get_level_editor_ent_strings();

void toggle_level_editor_hud(Uint8 toggle);
void set_level_editor_state(EditorState state);


#endif
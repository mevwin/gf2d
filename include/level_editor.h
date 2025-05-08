#ifndef __LEVEL_EDITOR_H__
#define __LEVEL_EDITOR_H__

#include "gfc_vector.h"
#include "entity.h"

typedef enum EditorState_E {
	EDITOR_SELECT_MODE,
	EDITOR_EXISTING_LEVELS,
	EDITOR_EDITING,
	EDITOR_ROOMS_MENU
}EditorState;

typedef enum EditorDrawMode_E {
	EDITOR_DRAW_ENTITY,
	EDITOR_DRAW_TERRAIN
}EditorDrawMode;

typedef enum EditorTrnMode_E {
	EDITOR_TRN_GROUND,
	EDITOR_TRN_PLAT
}EditorTrnMode;

// replica of Room from level.h that allows for dynamic number of level elements
typedef struct EditorRoom_S {
	Uint8				_inuse;
	Uint8				entered;

	GFC_TextWord		name;
	GFC_Vector2D		player_spawn;

	GFC_List*			grounds;
	GFC_List*			platforms;
	GFC_List*			level_spawns;
	GFC_List*			transitions;

	GFC_List*			enemy_list;
	GFC_List*			item_list;
	GFC_List*			hazard_list;
}EditorRoom;

void level_editor_init();
void level_editor_update();
void level_editor_close();

void level_editor_save_new_level();

//void level_editor_load_existing_level();
void initialize_dummy_level();
void level_editor_create_new_room();
void level_editor_remove_room();
void free_editor_level();

void initialize_level_previews();
void free_level_previews();

/**
* @brief pre-load all entities in their default states
*/
void initialize_level_editor_all_entities();
void initialize_level_editor_entity_previews();
void change_level_editor_list_type(EntityType new);

void level_editor_draw_level_previews();
void level_editor_draw_entity_preview_region();

void level_editor_inc_level_preview_pg();
void level_editor_dec_level_preview_pg();
void level_editor_next_list_type();
void level_editor_prev_list_type();
void level_editor_next_ent();
void level_editor_prev_ent();
void level_editor_inc_room_index();
void level_editor_dec_room_index();

EditorState get_level_editor_state();
GFC_Vector2D get_level_editor_room_layout();
Uint8 get_level_editor_hud_toggle();
EntityType get_level_editor_list_type();
GFC_TextWord* get_level_editor_ent_strings();
EditorDrawMode get_level_editor_draw_mode();
EditorTrnMode get_level_editor_trn_mode();
Uint8 get_level_editor_show_controls();
EditorRoom* get_current_editor_room();

void toggle_level_editor_hud(Uint8 toggle);
void set_level_editor_state(EditorState state);
void set_level_editor_draw_mode(EditorDrawMode mode);
void set_level_editor_trn_mode(EditorTrnMode mode);

#endif
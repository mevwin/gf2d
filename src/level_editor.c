#include "simple_logger.h"
#include "gfc_config.h"
#include "world.h"
#include "font.h"
#include "ui.h"
#include "level_editor.h"
#include "mouse.h"
#include "collisions.h"
#include "player.h"
#include "enemy.h"
#include "item.h"

typedef struct EditorManager_S {
    EditorState     state;

    int             room_count;
    GFC_Vector2D    room_layout;

    // level previews
    Uint8           preview_pos_count;
    GFC_Vector2D*   level_preview_positions;
    GFC_List*       level_preview_buttons;
    Uint8           preview_pos_page;
}EditorManager;

static EditorManager editor = { 0 };

void level_editor_init(){
    SJson* config, *entry;
    int i;

    config = sj_load("config/level_editor.cfg");
    if (!config) {
        slog("cannot load level_editor.cfg");
        change_world_state(WORLD_MAINMENU);
        return;
    }

    level_manager_init("config/levels.cfg");

    // level preview set up
    editor.preview_pos_page = 0;
    editor.level_preview_buttons = gfc_list_new();
    entry = sj_object_get_value(config, "level_preview_positions");
    editor.preview_pos_count = entry->v.array->count;
    editor.level_preview_positions = gfc_allocate_array(sizeof(GFC_Vector2D), editor.preview_pos_count);
    for (i = 0; i < editor.preview_pos_count; i++) {
        sj_value_as_vector2d(sj_array_nth(entry, i), &editor.level_preview_positions[i]);
    }
    
    editor.state = EDITOR_SELECT_MODE;
    editor.room_count = 1;
    editor.room_layout = gfc_vector2d(0, 0);
    //sj_free(config);
}

void level_editor_close() {
    level_manager_close();

    free(editor.level_preview_positions);

    free_level_previews();
    gfc_list_delete(editor.level_preview_buttons);

    memset(&editor, 0, sizeof(EditorManager));
}

void level_editor_update() {

}

void initialize_level_previews() {
    GFC_List* level_paths;
    GFC_TextLine buffer;
    TextLine* txt;
    Button* button;
    SJson* file, *level;
    int i, j;

    // init level select buttons
    level_paths = get_level_paths();
    for (i = 0, j = 0; i < level_paths->count; i++, j++) {
        if (j == 6) j = 0;

        strcpy(buffer, (const char*)gfc_list_nth(level_paths, i));
        file = sj_load(buffer);
        if (!file) continue;
        level = sj_object_get_value(file, "level");

        //level_editor_level_preview(file);
        txt = create_textline(
            sj_object_get_string(level, "name"),
            FONT_STANDARD,
            FONT_SIZE_MEDIUM,
            GFC_COLOR_WHITE,
            gfc_vector2d(-1, -1)
        );

        button = create_button(
            BUTTON_MENU,
            txt,
            editor.level_preview_positions[j],
            gfc_color8(0, 120, 0, 240)
        );

        gfc_list_append(editor.level_preview_buttons, button);

        sj_free(file);
    }
}

void free_level_previews() {
    Button* b;

    for (int i = 0; i < editor.level_preview_buttons->count; i++) {
        b = (Button*)gfc_list_nth(editor.level_preview_buttons, i);
        if (!b) continue;
        free(b);
    }

    gfc_list_clear(editor.level_preview_buttons);
}

void level_editor_draw_level_previews() {
    Button* b;
    int i, j;

    for (i = 0, j = editor.preview_pos_page * 6; i < 6; i++, j++) {
        b = (Button*) gfc_list_nth(editor.level_preview_buttons, j);
        if (!b) continue;

        draw_button(b, -2);

        // handle input
        if (mouse_in_rect(b->region) && mouse_button_pressed(MOUSE_LEFT_CLICK)){
            
            //editor.state = EDITOR_EDITING;
        }
    }
}

void level_editor_inc_level_preview_pg() {
    editor.preview_pos_page++;
    if (editor.preview_pos_page * 6 > editor.level_preview_buttons->count)
        editor.preview_pos_page--;
}

void level_editor_dec_level_preview_pg() {
    if (editor.preview_pos_page)
        editor.preview_pos_page--;
}

EditorState get_level_editor_state() {
    return editor.state;
}

int get_level_editor_room_count() {
    return editor.room_count;
}

GFC_Vector2D get_level_editor_room_layout() {
    return editor.room_layout;
}

void set_level_editor_state(EditorState state) {
    editor.state = state;
}
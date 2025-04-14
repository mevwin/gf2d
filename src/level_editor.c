#include "simple_logger.h"
#include "gfc_config.h"
#include "world.h"
#include "font.h"
#include "level_editor.h"
#include "collisions.h"
#include "player.h"
#include "enemy.h"
#include "item.h"

typedef struct EditorManager_S {
    EditorState     state;

    int             room_count;
}EditorManager;

static EditorManager editor = { 0 };

void level_editor_init(){
    SJson* config, *entry;

    config = sj_load("config/level_editor.cfg");
    if (!config) {
        slog("cannot load level_editor.cfg");
        change_world_state(WORLD_MAINMENU);
        return;
    }

    //sj_object_get_string(config, "menu");

    
    editor.state = EDITOR_ROOM_INIT;
    editor.room_count = 0;
    sj_free(config);
}

void level_editor_close() {

    memset(&editor, 0, sizeof(EditorManager));
}

void level_editor_update() {
    GFC_TextWord room_count;
    GFC_Vector2D offset;

    switch (editor.state) {
        case EDITOR_ROOM_INIT:
            gfc_word_sprintf(room_count, "%d", editor.room_count);
            offset = gfc_vector2d(620, 270);

            font_display_text(
                room_count,
                FONT_STANDARD,
                FONT_SIZE_LARGE,
                GFC_COLOR_WHITE,
                0,
                &offset,
                NULL);

            break;
    }
}

void level_editor_inc_room_count() {
    editor.room_count++;
}

void level_editor_dec_room_count() {
    if (!editor.room_count) return;
    editor.room_count--;
}

EditorState get_level_editor_state() {
    return editor.state;
}

void set_level_editor_state(EditorState state) {
    editor.state = state;
}

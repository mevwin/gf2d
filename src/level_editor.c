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
    GFC_Vector2D    room_layout;
}EditorManager;

static EditorManager editor = { 0 };

void level_editor_init(){
    //SJson* config, *entry;

    /*
    config = sj_load("config/level_editor.cfg");
    if (!config) {
        slog("cannot load level_editor.cfg");
        change_world_state(WORLD_MAINMENU);
        return;
    }
    */

    //sj_object_get_string(config, "menu");

    
    editor.state = EDITOR_ROOM_INIT;
    editor.room_count = 1;
    editor.room_layout = gfc_vector2d(0, 0);
    //sj_free(config);
}

void level_editor_close() {

    memset(&editor, 0, sizeof(EditorManager));
}

void level_editor_update() {

}

void predict_room_layout() {
    int x = editor.room_count;

    // prioritze length over width
    editor.room_layout.x = (float) x * x;
    editor.room_layout.y = (float) x * x;
}

void level_editor_inc_room_count() {
    editor.room_count++;
}

void level_editor_dec_room_count() {
    if (editor.room_count == 1) return;
    editor.room_count--;
}

void level_editor_inc_room_layout_x() {
    editor.room_layout.x += 1.0f;
}

void level_editor_dec_room_layout_x() {
    if (editor.room_layout.x == 1.0f) return;
    editor.room_layout.x -= 1.0f;
}

void level_editor_inc_room_layout_y() {
    editor.room_layout.y += 1.0f;
}

void level_editor_dec_room_layout_y() {
    if (editor.room_layout.y == 1.0f) return;
    editor.room_layout.y -= 1.0f;
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
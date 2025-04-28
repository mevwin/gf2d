#include "simple_logger.h"
#include "gfc_config.h"
#include "gf2d_draw.h"
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

    // entity previews
    Uint8           ent_type_count;
    GFC_TextWord*   ent_strings;

    EntityType      entity_preview_type;
    Uint8           entity_preview_index;
    GFC_List*       entity_preview_list;
    GFC_List*       all_entities;               // pre-loaded list of entities

    GFC_Vector2D    entity_name_offset;
    GFC_Vector2D    entity_preview_offset;
    GFC_Rect        entity_preview_region;

    // level previews
    Uint8           preview_pos_count;
    GFC_Vector2D*   level_preview_positions;
    GFC_List*       level_preview_buttons;
    Uint8           preview_pos_page;

    // flags
    Uint8           toggle_hud;
    Uint8           holding_entity;
}EditorManager;

static EditorManager editor = { 0 };

void level_editor_init(){
    SJson* config, *entry;
    GFC_Vector4D vec;
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

    // entity type strings
    entry = sj_object_get_value(config, "entity_types");
    editor.ent_type_count = entry->v.array->count;
    editor.ent_strings = gfc_allocate_array(sizeof(GFC_TextWord), editor.ent_type_count);
    for (i = 0; i < editor.ent_type_count; i++) {
        strcpy(editor.ent_strings[i], sj_get_string_value(sj_array_nth(entry, i)));
    }
    
    editor.entity_preview_type = ENEMY;
    editor.entity_preview_list = gfc_list_new();
    editor.all_entities = gfc_list_new();
    editor.entity_preview_index = 0;
    
    // other values
    sj_object_get_vector2d(config, "entity_name_offset", &editor.entity_name_offset);
    sj_object_get_vector2d(config, "entity_preview_offset", &editor.entity_preview_offset);
    sj_object_get_vector4d(config, "enttiy_preview_region", &vec);
    editor.entity_preview_region = gfc_rect_from_vector4(vec);

    editor.state = EDITOR_SELECT_MODE;
    editor.room_count = 1;
    editor.room_layout = gfc_vector2d(0, 0);
    editor.toggle_hud = 1;

    sj_free(config);
}

void level_editor_close() {
    Entity* ent;
    int i;

    level_manager_close();

    free(editor.ent_strings);
    free(editor.level_preview_positions);

    free_level_previews();
    gfc_list_delete(editor.level_preview_buttons);

    gfc_list_delete(editor.entity_preview_list);
    for (i = 0; i < editor.all_entities->count; i++) {
        ent = (Entity*) gfc_list_nth(editor.all_entities, i);
        entity_free(ent);
    }
    gfc_list_delete(editor.all_entities);
    
    memset(&editor, 0, sizeof(EditorManager));
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

void initialize_level_editor_all_entities() {
    SJson* def, *list, *entry;
    int i;

    // load enemies
    def = sj_load("def/enemy.def");
    if (!def) {
        slog("enemy.def not found by level editor");
        level_editor_close();
        change_world_state(WORLD_MAINMENU);
        return;
    }
    list = sj_object_get_value(def, "enemy_list");
    if (list) {
        for (i = 0; i < list->v.array->count; i++) {
            entry = sj_array_nth(list, i);
            gfc_list_append(editor.all_entities, enemy_dummy_spawn(entry, i, editor.entity_preview_offset));
        }
    }
    sj_free(def);

    // load items
    def = sj_load("def/item.def");
    if (def) {
        list = sj_object_get_value(def, "items");
        if (list) {
            for (i = 0; i < list->v.array->count; i++) {
                entry = sj_array_nth(list, i);
                gfc_list_append(editor.all_entities, item_dummy_spawn(entry, i, editor.entity_preview_offset));
            }
        }
        sj_free(def);
    }
    else slog("item.def not found by level editor");

    // load hazards (TODO)
}

void initialize_level_editor_entity_previews() {
    Entity* ent;
    int i;

    for (i = 0; i < editor.all_entities->count; i++) {
        ent = (Entity*) gfc_list_nth(editor.all_entities, i);
        if (!ent || ent->type != editor.entity_preview_type) continue;

        gfc_list_append(editor.entity_preview_list, ent);
    }

    ent = (Entity*)gfc_list_nth(editor.entity_preview_list, editor.entity_preview_index);
    if (ent) ent->draw_flag = 1;

    //slog("%i", editor.entity_preview_list->count);
}

void change_level_editor_list_type(EntityType new) {
    Entity* ent;
    
    editor.entity_preview_type = new;
    
    // reset draw flag for entity being drawn from preview list
    ent = (Entity*) gfc_list_nth(editor.entity_preview_list, editor.entity_preview_index);
    if (ent) ent->draw_flag = 0;

    editor.entity_preview_index = 0;

    // change list contents
    gfc_list_clear(editor.entity_preview_list);
    initialize_level_editor_entity_previews();
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

void level_editor_update() {
    switch (editor.state) {
        case EDITOR_EDITING:
            // input checks
            if (gfc_input_command_pressed("display")) {
                if (editor.toggle_hud) editor.toggle_hud = 0;
                else editor.toggle_hud = 1;
            }

            break;
    }
}

void level_editor_draw_level_previews() {
    Button* b;
    int i, j;

    for (i = 0, j = editor.preview_pos_page * 6; i < 6; i++, j++) {
        b = (Button*) gfc_list_nth(editor.level_preview_buttons, j);
        if (!b) continue;

        draw_button(b, -2);

        // TODO: handle input by loading level from json
        if (mouse_in_rect(b->region) && mouse_button_pressed(MOUSE_LEFT_CLICK)){
            
            //editor.state = EDITOR_EDITING;
        }
    }
}

void level_editor_draw_entity_preview_region() {
    Entity* ent;
    ItemData* i_data;
    GFC_TextWord text;

    gf2d_draw_rect_filled(editor.entity_preview_region, gfc_color8(120, 120, 120, 120));
    
    // print entity name
    ent = (Entity*)gfc_list_nth(editor.entity_preview_list, editor.entity_preview_index);
    if (!ent) return;

    switch (ent->type) {
        case ENEMY:
            font_display_text(ent->name,
                FONT_STANDARD,
                FONT_SIZE_SMALL,
                GFC_COLOR_WHITE,
                0,
                &editor.entity_name_offset,
                NULL
            );

            break;

        case HAZARD:

            break;

        case ITEM:
            i_data = ent->data;

            font_display_text(
                i_data->effect_value.text,
                FONT_STANDARD,
                FONT_SIZE_SMALL,
                GFC_COLOR_WHITE,
                0,
                &editor.entity_name_offset,
                NULL
            );

            break;
    }
}

void level_editor_inc_level_preview_pg() {
    if (editor.preview_pos_page * 6 <= editor.level_preview_buttons->count)
        editor.preview_pos_page++;
}

void level_editor_dec_level_preview_pg() {
    if (editor.preview_pos_page)
        editor.preview_pos_page--;
}

void level_editor_next_list_type() {
    if (editor.entity_preview_type == HAZARD)
        change_level_editor_list_type(ENEMY);
    else
        change_level_editor_list_type(editor.entity_preview_type + 1);
}

void level_editor_prev_list_type() {
    if (editor.entity_preview_type == ENEMY)
        change_level_editor_list_type(HAZARD);
    else
        change_level_editor_list_type(editor.entity_preview_type - 1);
}

void level_editor_next_ent() {
    Entity* ent;

    ent = (Entity*)gfc_list_nth(editor.entity_preview_list, editor.entity_preview_index);
    if (ent) ent->draw_flag = 0;

    if (editor.entity_preview_index < editor.entity_preview_list->count - 1)
        editor.entity_preview_index++;
    else
        editor.entity_preview_index = 0;

    ent = (Entity*) gfc_list_nth(editor.entity_preview_list, editor.entity_preview_index);
    if (ent) ent->draw_flag = 1;

    //slog("%i", editor.entity_preview_index);
}

void level_editor_prev_ent() {
    Entity* ent;

    ent = (Entity*)gfc_list_nth(editor.entity_preview_list, editor.entity_preview_index);
    if (ent) ent->draw_flag = 0;

    if (editor.entity_preview_index)
        editor.entity_preview_index--;
    else
        editor.entity_preview_index = editor.entity_preview_list->count - 1;

    ent = (Entity*) gfc_list_nth(editor.entity_preview_list, editor.entity_preview_index);
    if (ent) ent->draw_flag = 1;

    //slog("%i", editor.entity_preview_index);
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

Uint8 get_level_editor_hud_toggle() {
    return editor.toggle_hud;
}

EntityType get_level_editor_list_type() {
    return editor.entity_preview_type;
}

GFC_TextWord* get_level_editor_ent_strings() {
    return editor.ent_strings;
}

void set_level_editor_state(EditorState state) {
    editor.state = state;
}

void toggle_level_editor_hud(Uint8 toggle) {
    editor.toggle_hud = toggle;
}
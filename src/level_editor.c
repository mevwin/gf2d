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

    // flags
    Uint8           player_spawn_set;
    Uint8           show_controls;
    Uint8           toggle_hud;
    Uint8           holding;
    Uint8           drawing_trn;
    EditorDrawMode  drawMode;
    EditorTrnMode   trnMode;

    // draw info
    GFC_Vector2D    draw_trn_start;
    GFC_Vector2D    draw_trn_end;
    GFC_Rect        trn_preview;

    // level data
    Level*          level;
    GFC_List*       room_buffer;
    Uint8			room_index;
    GFC_Vector2D    room_layout;
    Entity*         player;
}EditorManager;

static EditorManager editor = { 0 };

EditorRoom* create_empty_room();
EditorRoom* get_current_editor_room();
void reset_level_editor_entity_previews();

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
    /*
    editor.preview_pos_page = 0;
    editor.level_preview_buttons = gfc_list_new();

    entry = sj_object_get_value(config, "level_preview_positions");
    editor.preview_pos_count = entry->v.array->count;
    editor.level_preview_positions = gfc_allocate_array(sizeof(GFC_Vector2D), editor.preview_pos_count);
    for (i = 0; i < editor.preview_pos_count; i++) {
        sj_value_as_vector2d(sj_array_nth(entry, i), &editor.level_preview_positions[i]);
    }
    */

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
    sj_object_get_vector4d(config, "entity_preview_region", &vec);
    editor.entity_preview_region = gfc_rect_from_vector4(vec);

    editor.state = EDITOR_SELECT_MODE;
    editor.toggle_hud = 1;
    editor.drawMode = EDITOR_DRAW_TERRAIN;
    editor.trnMode = EDITOR_TRN_GROUND;
    editor.show_controls = 1;
    editor.trn_preview = gfc_rect(0, 0, 0, 0);
    editor.player_spawn_set = 0;
    editor.player = NULL;

    sj_free(config);
}

void level_editor_close() {
    level_manager_close();

    free(editor.ent_strings);

    gfc_list_delete(editor.entity_preview_list);
    gfc_list_delete(editor.all_entities);
    
    memset(&editor, 0, sizeof(EditorManager));
}

void level_editor_save_new_level() {
    // LAST SESSION: save a new level

}

EditorRoom* create_empty_room() {
    EditorRoom* r;
    int i;

    r = gfc_allocate_array(sizeof(EditorRoom), 1);
    i = 100 + gfc_random_int(899);
    gfc_word_sprintf(r->name, "ROOM #%d", i);
    r->player_spawn = gfc_vector2d(-1, -1);

    r->grounds = gfc_list_new();
    r->platforms = gfc_list_new();
    r->level_spawns = gfc_list_new();
    r->transitions = gfc_list_new();

    r->enemy_list = gfc_list_new();
    r->item_list = gfc_list_new();
    r->hazard_list = gfc_list_new();

    return r;
}

void level_editor_create_new_room() {
    EditorRoom* r;

    r = create_empty_room();
    gfc_list_append(editor.room_buffer, r);
}

void level_editor_remove_room() {
    EditorRoom* r;

    if (editor.room_buffer->count == 1) {
        slog("must have at least one room");
        return;
    }

    r = (EditorRoom*) gfc_list_nth(editor.room_buffer, editor.room_index - 1);
    if (!r) return;

    gfc_list_delete_data(editor.room_buffer, r);
    free(r);

    editor.room_index = 1;
}

void initialize_dummy_level() {
    // generate a blank level
    editor.level = gfc_allocate_array(sizeof(Level), 1);
    gfc_word_cpy(editor.level->name, "new_level");
    editor.level->level_type = LEVEL_TYPE_REGULAR;
    editor.level->objective = LEVEL_OBJ_KILL_ALL_ENEMIES;
    editor.level->start_room = 1;
    editor.room_buffer = gfc_list_new();

    // initialize a default room
    editor.room_index = 1;
    editor.room_layout = gfc_vector2d(1, 1);
    level_editor_create_new_room();
}

void free_editor_level() {
    EditorRoom* room;
    Ground* g;
    int j;

    free_all_entities();
    for (int i = 0; i < editor.room_buffer->count; i++) {
        room = (EditorRoom*) gfc_list_nth(editor.room_buffer, i);
        if (!room) continue;

        for (j = 0; j < room->grounds->count; j++) {
            g = (Ground*) gfc_list_nth(room->grounds, j);
            free(g);
        }
        gfc_list_delete(room->grounds);

        gfc_list_delete(room->platforms);
        gfc_list_delete(room->level_spawns);
        gfc_list_delete(room->transitions);

        gfc_list_delete(room->enemy_list);
        gfc_list_delete(room->item_list);
        gfc_list_delete(room->hazard_list);

        free(room);
    }

    gfc_list_delete(editor.room_buffer);

    free(editor.level);
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

    ent = (Entity*) gfc_list_nth(editor.entity_preview_list, editor.entity_preview_index);
    if (ent) ent->draw_flag = 1;
}

void change_level_editor_list_type(EntityType new) {
    editor.entity_preview_type = new;
    reset_level_editor_entity_previews();
    editor.entity_preview_index = 0;
    initialize_level_editor_entity_previews();
}

void reset_level_editor_entity_previews() {
    Entity* ent;

    // reset draw flag for entity being drawn from preview list
    ent = (Entity*)gfc_list_nth(editor.entity_preview_list, editor.entity_preview_index);
    if (ent) ent->draw_flag = 0;

    // clear list contents
    gfc_list_clear(editor.entity_preview_list);
}

void draw_level_editor_entities(GFC_List* list, GFC_Vector2D m_pos) {
    Entity* ent;

    for (int i = 0; i < list->count; i++) {
        ent = (Entity*)gfc_list_nth(list, i);
        if (!ent) continue;

        entity_draw(ent);
        gf2d_draw_rect(ent->hurtbox.s.r, GFC_COLOR_RED);
        update_hurtbox(ent);
        update_boundbox(ent);
    }
}

void handle_level_editor_mouse_input(GFC_List* list, GFC_Vector2D m_pos) {
    Entity* ent;

    for (int i = 0; i < list->count; i++) {
        ent = (Entity*)gfc_list_nth(list, i);
        if (!ent) continue;

        // move around an entity
        if (!editor.toggle_hud && mouse_in_rect(ent->hurtbox.s.r)) {
            if (mouse_button_held(MOUSE_LEFT_CLICK))
                gfc_vector2d_copy(ent->position, m_pos);
            else if (mouse_button_held(MOUSE_MIDDLE_CLICK)) {
                gfc_list_delete_data(list, ent);
                entity_free(ent);
                return;
            }
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

    entity_draw(ent);
}

void level_editor_set_player_spawn() {
    if (editor.player_spawn_set) return;

    editor.player = player_spawn(gfc_vector2d(640, 300));
    editor.player_spawn_set = 1;
}

void level_editor_update() {
    Ground* g;
    EditorRoom* room;
    GFC_Vector2D m_pos;
    Entity* ent, *new_ent;
    EnemyData* e_data;
    ItemData* i_data;
    GFC_TextWord name;

    if (editor.state == EDITOR_ROOMS_MENU){
        if (gfc_input_command_pressed("rooms_menu")) {
            editor.state = EDITOR_EDITING;
            return;
        }
    }
    else if (editor.state == EDITOR_EDITING) {
        room = get_current_editor_room();
        m_pos = get_mouse_position();

        // input checks
        if (gfc_input_command_pressed("display")) {
            if (editor.toggle_hud) {
                if (editor.drawMode == EDITOR_DRAW_ENTITY)
                    reset_level_editor_entity_previews();
                editor.toggle_hud = 0;
            }
            else {
                if (editor.drawMode == EDITOR_DRAW_ENTITY)
                    initialize_level_editor_entity_previews();
                editor.toggle_hud = 1;
            }
        }

        if (gfc_input_command_pressed("change_draw_mode") && editor.toggle_hud) {
            if (editor.drawMode == EDITOR_DRAW_ENTITY) {
                reset_level_editor_entity_previews();
                editor.drawMode = EDITOR_DRAW_TERRAIN;
            }
            else if (editor.drawMode == EDITOR_DRAW_TERRAIN) {
                initialize_level_editor_entity_previews();
                editor.drawMode = EDITOR_DRAW_ENTITY;
            }
        }

        if (gfc_input_command_pressed("show_controls") && editor.toggle_hud) {
            if (editor.show_controls)
                editor.show_controls = 0;
            else
                editor.show_controls = 1;
        }

        if (gfc_input_command_pressed("rooms_menu") && editor.toggle_hud) {
            editor.state = EDITOR_ROOMS_MENU;
            return;
        }

        // draw checks/logic
        switch (editor.drawMode) {
            case EDITOR_DRAW_ENTITY:
                // create selected entity
                if (editor.toggle_hud && mouse_button_pressed(MOUSE_LEFT_CLICK) && mouse_in_rect(editor.entity_preview_region)) {
                    ent = (Entity*)gfc_list_nth(editor.entity_preview_list, editor.entity_preview_index);

                    if (ent) {
                        //slog("spawned entity");

                        // copy entity data
                        switch (ent->type) {
                            case ENEMY:
                                e_data = ent->data;

                                gfc_word_sprintf(name, "enemy%i", room->enemy_list->count + 1);
                                new_ent = enemy_spawn(e_data->type, name, gfc_vector2d(640, 360), NULL);

                                gfc_list_append(room->enemy_list, new_ent);

                                break;

                            case ITEM:
                                i_data = ent->data;

                                new_ent = item_spawn(ent->name, gfc_vector2d(640, 360));
                                gfc_list_append(room->item_list, new_ent);

                                break;

                            case HAZARD:

                                break;
                        }
                    }
                }

                // handle mouse inputs for entities
                handle_level_editor_mouse_input(room->enemy_list, m_pos);
                handle_level_editor_mouse_input(room->item_list, m_pos);
                // handle_level_editor_mouse_input(room->hazard_last, m_pos);

                if (editor.player && !editor.toggle_hud && mouse_in_rect(editor.player->hurtbox.s.r)) {
                    if (mouse_button_held(MOUSE_LEFT_CLICK)) {
                        gfc_vector2d_copy(editor.player->position, m_pos);
                        update_hurtbox(editor.player);
                        update_boundbox(editor.player);
                    }
                    else if (mouse_button_held(MOUSE_MIDDLE_CLICK)) {
                        entity_free(editor.player);
                        editor.player = NULL;
                        editor.player_spawn_set = 0;
                    }
                }

                break;

            case EDITOR_DRAW_TERRAIN:
                if (!editor.toggle_hud && mouse_button_pressed(MOUSE_LEFT_CLICK) && !editor.drawing_trn) {
                    editor.drawing_trn = 1;
                    editor.draw_trn_start = get_mouse_position();
                    //slog("%f, %f", editor.draw_trn_start.x, editor.draw_trn_start.y);
                }

                if (editor.drawing_trn) {
                    if (mouse_button_held(MOUSE_LEFT_CLICK)) {
                        editor.draw_trn_end = get_mouse_position();

                        editor.trn_preview = gfc_rect(editor.draw_trn_start.x,
                            editor.draw_trn_start.y,
                            editor.draw_trn_end.x - editor.draw_trn_start.x,
                            editor.draw_trn_end.y - editor.draw_trn_start.y
                        );

                        gf2d_draw_rect_filled(editor.trn_preview, gfc_color8(0, 120, 0, 120));
                    }
                    else {
                        //slog("%f, %f", editor.draw_trn_end.x, editor.draw_trn_end.y);
                        editor.draw_trn_end = get_mouse_position();
                        editor.trn_preview = gfc_rect(editor.draw_trn_start.x,
                            editor.draw_trn_start.y,
                            editor.draw_trn_end.x - editor.draw_trn_start.x,
                            editor.draw_trn_end.y - editor.draw_trn_start.y
                        );

                        // arrange rect values for positive offset/dimensions
                        if (editor.draw_trn_end.x < editor.draw_trn_start.x) {
                            editor.trn_preview.x = editor.draw_trn_end.x;
                            editor.trn_preview.w *= -1;
                        }

                        if (editor.draw_trn_end.y < editor.draw_trn_start.y) {
                            editor.trn_preview.y = editor.draw_trn_end.y;
                            editor.trn_preview.h *= -1;
                        }

                        // create terrain
                        switch (editor.trnMode) {
                            case EDITOR_TRN_GROUND:
                                g = create_ground(
                                    editor.trn_preview,
                                    gfc_vector2d(editor.trn_preview.x, editor.trn_preview.y),
                                    gfc_color(gfc_random(), gfc_random(), gfc_random(), 1),
                                    1,
                                    1);

                                gfc_list_append(room->grounds, g);

                                break;
                            case EDITOR_TRN_PLAT:

                                break;
                        }

                        editor.drawing_trn = 0;
                    }
                }

                break;
        }

        // draw entities
        draw_level_editor_entities(room->enemy_list, m_pos);
        draw_level_editor_entities(room->item_list, m_pos);
        if (editor.player) {
            entity_draw(editor.player);
            gf2d_draw_rect(editor.player->hurtbox.s.r, GFC_COLOR_GREEN);
        }

        // draw level and check each level element
        for (int i = 0; i < room->grounds->count; i++) {
            g = (Ground*)gfc_list_nth(room->grounds, i);
            if (!g) continue;

            gf2d_draw_rect_filled(g->dimensions, g->color);

            if (!editor.toggle_hud && mouse_in_rect(g->dimensions) && editor.drawMode == EDITOR_DRAW_TERRAIN) {
                if (mouse_button_pressed(MOUSE_MIDDLE_CLICK)) {
                    gfc_list_delete_data(room->grounds, g);
                    free(g);
                }
                else if (mouse_button_held(MOUSE_RIGHT_CLICK)) {
                    // reposition ground offset (TODO: FIX MULTIPLE GROUNDS BEING SELECTED)
                    g->dimensions.x = m_pos.x - (g->dimensions.w * 0.5f);
                    g->dimensions.y = m_pos.y - (g->dimensions.h * 0.5f);
                }
            }
        }
    }
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

void level_editor_inc_room_index() {
    if (editor.room_index < editor.room_buffer->count) {
        editor.room_index++;
    }
}

void level_editor_dec_room_index() {
    if (editor.room_index > 1)
        editor.room_index--;
}

EditorState get_level_editor_state() {
    return editor.state;
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

EditorDrawMode get_level_editor_draw_mode() {
    return editor.drawMode;
}

EditorTrnMode get_level_editor_trn_mode() {
    return editor.trnMode;
}

Uint8 get_level_editor_show_controls() {
    return editor.show_controls;
}

EditorRoom* get_current_editor_room() {
    return (EditorRoom*) gfc_list_nth(editor.room_buffer, editor.room_index - 1);
}

void toggle_level_editor_hud(Uint8 toggle) {
    editor.toggle_hud = toggle;
}

void set_level_editor_state(EditorState state) {
    editor.state = state;
}

void set_level_editor_draw_mode(EditorDrawMode mode) {
    editor.drawMode = mode;
}

void set_level_editor_trn_mode(EditorTrnMode mode) {
    editor.trnMode = mode;
}
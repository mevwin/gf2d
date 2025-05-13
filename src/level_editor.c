#include <direct.h>
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
#include "hazard.h"

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
    Entity*         player;
    Level*          level;
    GFC_List*       room_buffer;
    GFC_Vector2D    room_layout;
    Uint8			room_index;
    Uint8           room_tr_index;
  
    // room transitions
    Uint8           room_tr_id;
    GFC_List*       room_tr_list;       // for easier searching
}EditorManager;

static EditorManager editor = { 0 };

EditorRoom* create_empty_room();
EditorRoom* get_current_editor_room();
void reset_level_editor_entity_previews();
void update_room_transition_player_repo(RoomTransition* room_a);

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
    editor.room_index = 0;
    editor.room_tr_index = 0;
    
    editor.room_tr_id = 0;
    editor.room_tr_list = gfc_list_new();

    sj_free(config);
}

void level_editor_close() {
    level_manager_close();

    free(editor.ent_strings);

    gfc_list_delete(editor.room_tr_list);
    gfc_list_delete(editor.entity_preview_list);
    gfc_list_delete(editor.all_entities);
    
    memset(&editor, 0, sizeof(EditorManager));
}

Uint8 level_editor_save_new_level() {
    GFC_TextLine buffer;
    EditorRoom* room;
    SJson* file, *room_base, *list, *entry, *arr_entry, *array;
    Ground* g;
    Entity* ent;
    EnemyData* e_data;
    HazardData* h_data;
    RoomTransition* rt;
    Uint8 start_room = 0;
    int i, j, buf;

    // check if level have enough data in them
    if (!editor.player_spawn_set) {
        slog("player_spawn never set for level");
        return 0;
    }

    for (i = 0; i < editor.room_buffer->count; i++) {
        room = (EditorRoom*)gfc_list_nth(editor.room_buffer, i);
        if (!room) continue;

        // check if there is at least one ground
        if (!room->grounds->count) {
            slog("%s must have at least 1 ground", room->name);
            return 0;
        }
    }

    // save the rooms
    for (i = 0; i < editor.room_buffer->count; i++) {
        room = (EditorRoom*)gfc_list_nth(editor.room_buffer, i);
        if (!room) continue;

        /* save the rooms in appropriate folder */
        room_base = sj_object_new();

        sj_object_insert(room_base, "name", sj_new_str(room->name));
        sj_object_insert(room_base, "player_spawn", sj_vector2d_new(room->player_spawn));
        if (room->player_spawn.x != -1.0f && room->player_spawn.y != -1.0f)
            start_room = i + 1;

        list = sj_array_new();
        for (j = 0; j < room->grounds->count; j++) {
            g = (Ground*)gfc_list_nth(room->grounds, j);
            if (!g) continue;

            arr_entry = sj_object_new();

            array = sj_array_new();
            sj_array_append(array, sj_new_float(g->dimensions.x));
            sj_array_append(array, sj_new_float(g->dimensions.y));
            sj_array_append(array, sj_new_float(g->dimensions.w));
            sj_array_append(array, sj_new_float(g->dimensions.h));

            sj_object_insert(arr_entry, "rect", array);
            sj_object_insert(arr_entry, "color", sj_color_new(g->color));
            sj_object_insert(arr_entry, "walls", sj_new_uint8(1));
            sj_object_insert(arr_entry, "ceiling", sj_new_uint8(g->ceil_flag));

            sj_array_append(list, arr_entry);
        }
        sj_object_insert(room_base, "ground_list", list);

        list = sj_array_new();
        for (j = 0; j < room->platforms->count; j++) {
            // save later
        }
        sj_object_insert(room_base, "platform_list", list);

        list = sj_array_new();
        for (j = 0; j < room->enemy_list->count; j++) {
            ent = (Entity*)gfc_list_nth(room->enemy_list, j);
            if (!ent) continue;

            arr_entry = sj_object_new();

            e_data = ent->data;

            buf = e_data->type;
            sj_object_insert(arr_entry, "type", sj_new_int(buf));
            sj_object_insert(arr_entry, "name", sj_new_str(ent->name));
            sj_object_insert(arr_entry, "position", sj_vector2d_new(ent->position));

            sj_array_append(list, arr_entry);
        }
        entry = sj_object_new();
        sj_object_insert(entry, "list", list);
        sj_object_insert(room_base, "enemies", entry);

        list = sj_array_new();
        for (j = 0; j < room->item_list->count; j++) {
            ent = (Entity*)gfc_list_nth(room->item_list, j);
            if (!ent) continue;

            arr_entry = sj_object_new();

            sj_object_insert(arr_entry, "name", sj_new_str(ent->name));
            sj_object_insert(arr_entry, "position", sj_vector2d_new(ent->position));

            sj_array_append(list, arr_entry);
        }
        sj_object_insert(room_base, "items", list);

        // TODO: HAZARDS
        list = sj_array_new();
        for (j = 0; j < room->hazard_list->count; j++) {
            ent = (Entity*) gfc_list_nth(room->hazard_list, j);
            if (!ent) continue;

            arr_entry = sj_object_new();

            h_data = ent->data;

            buf = h_data->h_type;
            sj_object_insert(arr_entry, "type", sj_new_int(buf));
            sj_object_insert(arr_entry, "name", sj_new_str(ent->name));
            sj_object_insert(arr_entry, "position", sj_vector2d_new(ent->position));
            sj_array_append(list, arr_entry);
        }
        sj_object_insert(room_base, "hazards", list);

        list = sj_array_new();
        for (j = 0; j < room->transitions->count; j++) {
            rt = (RoomTransition*) gfc_list_nth(room->transitions, j);
            if (!rt) continue;

            arr_entry = sj_object_new();

            array = sj_array_new();
            sj_array_append(array, sj_new_float(rt->region.x));
            sj_array_append(array, sj_new_float(rt->region.y));
            sj_array_append(array, sj_new_float(rt->region.w));
            sj_array_append(array, sj_new_float(rt->region.h));

            sj_object_insert(arr_entry, "region", array);
            sj_object_insert(arr_entry, "player_repo", sj_vector2d_new(rt->player_repo));
            sj_object_insert(arr_entry, "room_num", sj_new_uint8(rt->room_num));
            sj_object_insert(arr_entry, "locked", sj_new_uint8(rt->locked));
            sj_object_insert(arr_entry, "id", sj_new_int(rt->id));

            sj_array_append(list, arr_entry);
        }
        sj_object_insert(room_base, "room_transitions", list);

        file = sj_object_new();
        sj_object_insert(file, "room", room_base);

        // make folder
        sprintf(buffer, "levels/%s", editor.level->name);
        _mkdir(buffer);

        sprintf(buffer, "%s/room%d.def", buffer, i + 1);
        sj_save(file, buffer);
        //sj_free(room_file);
    }

    // save the level
    entry = sj_object_new();

    sj_object_insert(entry, "name", sj_new_str(editor.level->name));

    buf = (int)editor.level->level_type;
    sj_object_insert(entry, "type", sj_new_int(buf));

    buf = (int)editor.level->objective;
    sj_object_insert(entry, "obj", sj_new_int(buf));

    sj_object_insert(entry, "room_count", sj_new_uint32(editor.room_buffer->count));
    sj_object_insert(entry, "start_room", sj_new_int(start_room));

    editor.room_layout = gfc_vector2d(5, 3);
    sj_object_insert(entry, "layout_dimen", sj_vector2d_new(editor.room_layout));

    buf = 1;
    list = sj_array_new();
    for (i = 0; i < editor.room_layout.y; i++) {
        array = sj_array_new();
        for (j = 0; j < editor.room_layout.x; j++) {
            if (buf <= editor.room_buffer->count) {
                sj_array_append(array, sj_new_int(buf));
                buf++;
            }
            else sj_array_append(array, sj_new_int(0));
        }
        sj_array_append(list, array);
    }
    sj_object_insert(entry, "layout", list);

    file = sj_object_new();
    sj_object_insert(file, "level", entry);

    sprintf(buffer, "levels/%s.def", editor.level->name);
    sj_save(file, buffer);

    // update list on levels.cfg
    file = sj_load("config/levels.cfg");
    array = sj_object_get_value(file, "list");
    sj_array_append(array, sj_new_str(buffer));
    sj_save(file, "config/levels.cfg");
    return 1;
}

EditorRoom* create_empty_room() {
    EditorRoom* r;
    int i;

    r = gfc_allocate_array(sizeof(EditorRoom), 1);
    i = 100 + gfc_random_int(899);
    gfc_word_sprintf(r->name, "ROOM #%d", i); // temporary name for editing purposes
    r->player_spawn = gfc_vector2d(-1, -1);

    r->grounds = gfc_list_new();
    r->platforms = gfc_list_new();
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

void level_editor_free_room(EditorRoom* room) {
    Ground* g;
    Platform* p;
    LevelSpawn* ls;
    RoomTransition* rt;
    Entity* e;
    int i;

    if (!room) return;

    // free all assets saved
    for (i = 0; i < room->grounds->count; i++) {
        g = (Ground*) gfc_list_nth(room->grounds, i);
        if (!g) continue;
        free(g);
    }
    gfc_list_delete(room->grounds);

    for (i = 0; i < room->platforms->count; i++) {
        p = (Ground*) gfc_list_nth(room->platforms, i);
        if (!p) continue;
        free(p);
    }
    gfc_list_delete(room->platforms);

    for (i = 0; i < room->transitions->count; i++) {
        rt = (RoomTransition*) gfc_list_nth(room->transitions, i);
        if (!rt) continue;
        free(rt);
    }
    gfc_list_delete(room->transitions);

    for (i = 0; i < room->enemy_list->count; i++) {
        e = (Entity*) gfc_list_nth(room->enemy_list, i);
        if (!e) continue;
        entity_free(e);
    }
    gfc_list_delete(room->enemy_list);

    for (i = 0; i < room->item_list->count; i++) {
        e = (Entity*) gfc_list_nth(room->item_list, i);
        if (!e) continue;
        entity_free(e);
    }
    gfc_list_delete(room->item_list);

    for (i = 0; i < room->hazard_list->count; i++) {
        e = (Entity*) gfc_list_nth(room->hazard_list, i);
        if (!e) continue;
        entity_free(e);
    }
    gfc_list_delete(room->hazard_list);

    free(room);
}

void level_editor_remove_room() {
    EditorRoom* r;

    if (editor.room_buffer->count == 1) {
        slog("must have at least one room");
        return;
    }

    r = (EditorRoom*) gfc_list_nth(editor.room_buffer, editor.room_index);
    if (r) {
        gfc_list_delete_data(editor.room_buffer, r);
        level_editor_free_room(r);
    }
    editor.room_index = 0;
}

void initialize_dummy_level() {
    editor.level = gfc_allocate_array(sizeof(Level), 1);
    sprintf(editor.level->name, "level%i", get_level_list_count() + 1);
    editor.level->level_type = LEVEL_TYPE_REGULAR;
    editor.level->objective = LEVEL_OBJ_KILL_ALL_ENEMIES;
    editor.level->start_room = 1;
    editor.room_buffer = gfc_list_new();

    // initialize a default room
    editor.room_layout = gfc_vector2d(1, 1);
    level_editor_create_new_room();
}

void free_editor_level() {
    EditorRoom* room;
    Ground* g;
    int j;

    if (!editor.level) return;

    free_all_entities();
    for (int i = 0; i < editor.room_buffer->count; i++) {
        room = (EditorRoom*) gfc_list_nth(editor.room_buffer, i);
        if (!room) continue;
        level_editor_free_room(room);
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

    // load hazards
    def = sj_load("def/hazard.def");
    if (def) {
        list = sj_object_get_value(def, "hazard_list");
        if (list) {
            for (i = 0; i < list->v.array->count; i++) {
                entry = sj_array_nth(list, i);
                gfc_list_append(editor.all_entities, hazard_dummy_spawn(entry, i, editor.entity_preview_offset));
            }
        }
    }
    else slog("hazard.def not found by level editor");
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
    //if (ent) ent->draw_flag = 1;
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
    HazardData* h_data;

    for (int i = 0; i < list->count; i++) {
        ent = (Entity*)gfc_list_nth(list, i);
        if (!ent) continue;

        if (ent->type == HAZARD) {
            h_data = ent->data;

            if (h_data->h_type == HAZARD_GEYSER)
                gf2d_draw_rect_filled(ent->hurtbox.s.r, h_data->color);
            
        }
        else {
            entity_draw(ent);
            gf2d_draw_rect(ent->hurtbox.s.r, GFC_COLOR_RED);
            update_hurtbox(ent);
            update_boundbox(ent);
        }
    }
}

void handle_level_editor_mouse_input(GFC_List* list, GFC_Vector2D m_pos) {
    Entity* ent;
    HazardData* h_data;

    for (int i = 0; i < list->count; i++) {
        ent = (Entity*)gfc_list_nth(list, i);
        if (!ent) continue;

        // move around an entity
        // TODO: worry about how position data is different for hazards
        if (!editor.toggle_hud && mouse_in_rect(ent->hurtbox.s.r)) {
            if (mouse_button_held(MOUSE_LEFT_CLICK)) {
                gfc_vector2d_copy(ent->position, m_pos);
                if (ent->type == HAZARD) {
                    h_data = ent->data;
                    if (h_data->h_type == HAZARD_GEYSER) {
                        ent->position.x -= ent->hurtbox.s.r.w * 0.5f;
                        ent->position.y -= ent->hurtbox.s.r.h * 0.5f;
                        update_geyser_hurtbox(ent);
                    }
                }
            }
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
    HazardData* h_data;

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
            entity_draw(ent);

            break;

        case HAZARD:
            font_display_text(ent->name,
                FONT_STANDARD,
                FONT_SIZE_SMALL,
                GFC_COLOR_WHITE,
                0,
                &editor.entity_name_offset,
                NULL
            );

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
            entity_draw(ent);

            break;
    }


}

void level_editor_set_player_spawn() {
    EditorRoom *room = get_current_editor_room();

    if (editor.player_spawn_set) return;
    
    editor.player = player_spawn(gfc_vector2d(640, 300));
    gfc_vector2d_copy(room->player_spawn, editor.player->position);
    editor.player_spawn_set = 1;
}

Uint8 level_editor_room_transition_enough_rooms() {
    if (editor.room_buffer->count == 1) {
        slog("need more than one room to add transitions");
        return 0;
    }
    else return 1;
}

void level_editor_display_room_transition_name() {
    EditorRoom* room;
    GFC_Vector2D vec;

    vec = gfc_vector2d(720, 300);

    if (editor.room_tr_index == editor.room_index)
        level_editor_inc_room_tr_index();

    room = (EditorRoom*) gfc_list_nth(editor.room_buffer, editor.room_tr_index);
    if (!room) return;

    font_display_text(
        room->name,
        FONT_STANDARD,
        FONT_SIZE_MEDIUM,
        GFC_COLOR_WHITE,
        0,
        &vec,
        NULL
    );
}

void level_editor_create_room_transition() {
    EditorRoom* curr_room = get_current_editor_room(), * new_room;
    RoomTransition* curr_to_new, * new_to_curr;

    // get rooms
    new_room = (EditorRoom*)gfc_list_nth(editor.room_buffer, editor.room_tr_index);
    if (!new_room) return;

    // create a transition between them (one for start room, one for the other)
    // NOTE: default position values
    curr_to_new = create_room_transition(
        gfc_rect(100, 100, 150, 150),
        gfc_vector2d(100, 100),
        editor.room_tr_index + 1,
        0,
        editor.room_tr_id
    );
    gfc_list_append(curr_room->transitions, curr_to_new);

    new_to_curr = create_room_transition(
        gfc_rect(100, 100, 150, 150),
        gfc_vector2d(100, 100),
        editor.room_index + 1,
        0,
        editor.room_tr_id
    );
    gfc_list_append(new_room->transitions, new_to_curr);

    gfc_list_append(editor.room_tr_list, curr_to_new);
    gfc_list_append(editor.room_tr_list, new_to_curr);

    editor.room_tr_id++;
}

void update_room_transition_player_repo(RoomTransition* room_a) {
    RoomTransition* rt;

    for (int i = 0; i < editor.room_tr_list->count; i++) {
        rt = (RoomTransition*) gfc_list_nth(editor.room_tr_list, i);
        if (!rt || room_a == rt || room_a->id != rt->id) continue;

        rt->player_repo.x = room_a->region.x + (room_a->region.w * 0.5f);
        rt->player_repo.y = room_a->region.y + (room_a->region.h * 0.5f);
        return;
    }
}

void level_editor_update() {
    Ground* g;
    EditorRoom* room;
    RoomTransition* rt;
    GFC_Vector2D m_pos;
    Entity* ent, *new_ent;
    EnemyData* e_data;
    ItemData* i_data;
    HazardData* h_data;
    GFC_TextWord name;
    int i;

    // bug fix
    if (editor.room_tr_index < 0) editor.room_tr_index = 0;

    if (editor.state == EDITOR_ROOMS_MENU){
        if (gfc_input_command_pressed("rooms_menu")) {
            editor.state = EDITOR_EDITING;
            return;
        }
    }
    else if (editor.state == EDITOR_TRANSITIONS_MENU) {

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
                                h_data = ent->data;

                                gfc_word_sprintf(name, "hazard%i", room->hazard_list->count + 1);
                                new_ent = hazard_spawn(h_data->h_type, gfc_vector2d(640, 360), name);

                                gfc_list_append(room->hazard_list, new_ent);

                                break;
                        }
                    }
                }

                // handle mouse inputs for entities
                handle_level_editor_mouse_input(room->enemy_list, m_pos);
                handle_level_editor_mouse_input(room->item_list, m_pos);
                handle_level_editor_mouse_input(room->hazard_list, m_pos);

                if (editor.player && !editor.toggle_hud && mouse_in_rect(editor.player->hurtbox.s.r) 
                    && room->player_spawn.x != -1.0f && room->player_spawn.y != -1.0f) 
                {
                    if (mouse_button_held(MOUSE_LEFT_CLICK)) {
                        gfc_vector2d_copy(editor.player->position, m_pos);
                        gfc_vector2d_copy(room->player_spawn, editor.player->position);
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

        // draw level and check each level element
        for (i = 0; i < room->grounds->count; i++) {
            g = (Ground*)gfc_list_nth(room->grounds, i);
            if (!g) continue;

            gf2d_draw_rect_filled(g->dimensions, g->color);

            // handle input
            if (!editor.toggle_hud && editor.drawMode == EDITOR_DRAW_TERRAIN && mouse_in_rect(g->dimensions)) {
                if (mouse_button_pressed(MOUSE_MIDDLE_CLICK)) {
                    gfc_list_delete_data(room->grounds, g);
                    free(g);
                }
                else if (mouse_button_held(MOUSE_RIGHT_CLICK)) {
                    // reposition ground offset (TODO: FIX MULTIPLE GROUNDS BEING SELECTED)
                    g->dimensions.x = m_pos.x - (g->dimensions.w * 0.5f);
                    g->dimensions.y = m_pos.y - (g->dimensions.h * 0.5f);

                    g->region.x = g->dimensions.x;
                    g->region.y = g->dimensions.x + g->dimensions.w;
                }
            }
        }

        // draw room transitions
        for (i = 0; i < room->transitions->count; i++) {
            rt = (RoomTransition*) gfc_list_nth(room->transitions, i);
            if (!rt) continue;

            gf2d_draw_rect_filled(rt->region, GFC_COLOR_BROWN);

            // handle input
            if (!editor.toggle_hud && editor.drawMode == EDITOR_DRAW_ENTITY && mouse_in_rect(rt->region)) {
                if (mouse_button_held(MOUSE_LEFT_CLICK) ) {
                    rt->region.x = m_pos.x - (rt->region.w * 0.5f);
                    rt->region.y = m_pos.y - (rt->region.h * 0.5f);

                    // change reposition data for corresponding transition
                    update_room_transition_player_repo(rt);
                }
            }
        }

        // draw entities
        draw_level_editor_entities(room->enemy_list, m_pos);
        draw_level_editor_entities(room->item_list, m_pos);
        draw_level_editor_entities(room->hazard_list, m_pos);
        if (editor.player && room->player_spawn.x != -1.0f && room->player_spawn.y != -1.0f) {
            entity_draw(editor.player);
            gf2d_draw_rect(editor.player->hurtbox.s.r, GFC_COLOR_GREEN);
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
    if (editor.room_index < editor.room_buffer->count - 1)
        editor.room_index++;
}

void level_editor_dec_room_index() {
    if (editor.room_index > 0)
        editor.room_index--;
}

void level_editor_inc_room_tr_index() {
    if (editor.room_tr_index < editor.room_buffer->count - 1) {
        editor.room_tr_index++;
        if (editor.room_tr_index == editor.room_index) {
            editor.room_tr_index++;
            if (editor.room_tr_index > editor.room_buffer->count - 1)
                editor.room_tr_index = 0;
        }
    }
}

void level_editor_dec_room_tr_index() {
    if (editor.room_tr_index > 0) {
        editor.room_tr_index--;
        if (editor.room_tr_index == editor.room_index) {
            editor.room_tr_index--;
            if (editor.room_tr_index < 0)
                editor.room_tr_index = editor.room_buffer->count - 1;
        }
    }
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
    return (EditorRoom*) gfc_list_nth(editor.room_buffer, editor.room_index);
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
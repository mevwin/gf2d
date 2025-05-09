#include "simple_logger.h"
#include "gfc_input.h"
#include "gf2d_draw.h"
#include "world.h"
#include "mouse.h"
#include "level_editor.h"
#include "level.h"
#include "ui.h"
#include "player.h"
#include "enemy.h"

typedef struct UIManager_S {
    GFC_List*       ui_list;
    int             active_button;

    // sprites
    Sprite*         menu_button;
    Sprite*         scroll_button;
    Sprite*         window;
}UIManager;

static UIManager ui_manager = { 0 };

void ui_system_close();
Menu* create_menu(const char* filename);
void delete_menu(Menu* win);

void create_window_from_json(Window* window, SJson* data);
void create_button_from_json(Button* button, SJson* data);
void create_bar_from_json(Bar* bar, SJson* data);
void create_textline_from_json(TextLine* txt, SJson* data);

void reset_window_toggles();
void draw_window(Window* win);
void draw_notif_windows(Menu* m);

void draw_player_hud(Menu* menu, GFC_List* enemy_list, PlayerData* p_data);
void draw_world_menu(Menu* menu, WorldState world_state);
void draw_editor_menu(Menu* menu, EditorState editor_state);
void draw_editor_hud(Menu* menu);
void draw_editor_hud_buttons(Menu* menu, Window* win);
void draw_editor_hud_special_menu(Menu* menu);

Uint32 find_button_index_from_wbd(Menu* menu, WindowButtonData* wbd);
Window* get_window_by_name(Window* list, Uint32 count, const char* name);

void window_ttl_advance(Window* win);
void menu_buttonList_advance(int min, int max, Window* win);
void menu_buttonList_go_back(int min, int max, Window* win);
void menu_check_input(Menu* menu, int min, int max, Window* win);

void ui_system_init(char* configFile) {
    SJson* config, *menu_list;
    char* path;
    int i;
    
    // initialize all menus
    config = sj_load(configFile);
    if (!config) return;

    ui_manager.ui_list = gfc_list_new();

    ui_manager.menu_button = gf2d_sprite_load_image(sj_object_get_string(config, "menu_button"));
    ui_manager.scroll_button = gf2d_sprite_load_image(sj_object_get_string(config, "scroll_button"));
    ui_manager.window = gf2d_sprite_load_image(sj_object_get_string(config, "window"));

    menu_list = sj_object_get_value(config, "menu_list");
    for (i = 0; i < menu_list->v.array->count; i++) {
        path = sj_get_string_value(sj_array_get_nth(menu_list, i));
        gfc_list_append(ui_manager.ui_list, create_menu(path));
    }

    ui_manager.active_button = 0;

    sj_free(config);
    atexit(ui_system_close);
}

void ui_system_close() {
    gfc_list_foreach(ui_manager.ui_list, delete_menu);
    gfc_list_delete(ui_manager.ui_list);

    gf2d_sprite_delete(ui_manager.menu_button);
    gf2d_sprite_delete(ui_manager.scroll_button);
    gf2d_sprite_delete(ui_manager.window);

    memset(&ui_manager, 0, sizeof(UIManager));
}

Menu* create_menu(const char* filename) {
    SJson* data, *win_data,*list;
    Menu* menu;
    char buffer[100];
    int i;

    data = sj_load(filename);
    if (!data) {
        slog("failed to load menu file");
        return NULL;
    }

    menu = gfc_allocate_array(sizeof(Menu), 1);
    if (!menu) {
        slog("failed to allocate data for menu");
        sj_free(data);
        return NULL;
    }

    win_data = sj_object_get_value(data, "menu");

    strcpy(menu->name, sj_object_get_string(win_data, "name"));
    strcpy(buffer, sj_object_get_string(win_data, "bg_sprite"));
    if (strcmp(buffer, "no sprite")) {
        menu->bg_sprite = gf2d_sprite_load_image(buffer);
        //slog("menu sprite initialized");
    }
    //else slog("no sprite provided");

    // create buttons
    sj_object_get_uint8(win_data, "buttonMax", &menu->buttonMax);
    if (menu->buttonMax) {
        list = sj_object_get_value(data, "buttons");
        menu->buttonMax = list->v.array->count;
        menu->buttonList = gfc_allocate_array(sizeof(Button), menu->buttonMax);

        sj_object_get_int(win_data, "buttonLayout", &i);    
        menu->button_layout = (ButtonLayout) i;

        for (i = 0; i < menu->buttonMax; i++) {
            create_button_from_json(&menu->buttonList[i], sj_array_get_nth(list, i));
        }
    }

    // create bars
    sj_object_get_uint8(win_data, "barMax", &menu->barMax);
    if (menu->barMax) {
        list = sj_object_get_value(data, "bars");
        menu->barMax = list->v.array->count;
        menu->barList = gfc_allocate_array(sizeof(Bar), menu->barMax);
        
        for (i = 0; i < menu->barMax; i++) {
            create_bar_from_json(&menu->barList[i], sj_array_get_nth(list, i));
        }
    }

    // create windows
    sj_object_get_uint8(win_data, "windowMax", &menu->windowMax);
    if (menu->windowMax) {
        list = sj_object_get_value(data, "windows");
        menu->windowMax = list->v.array->count;
        menu->windowList = gfc_allocate_array(sizeof(Window), menu->windowMax);
      
        for (i = 0; i < menu->windowMax; i++) {
            create_window_from_json(&menu->windowList[i], sj_array_get_nth(list, i));
        }
    }

    sj_free(data);
    return menu;
}

void create_button_from_json(Button* button, SJson* data) {
    if (!button || !data) {
        slog("couldn't create button");
        return;
    }

    // textline
    create_textline_from_json(&button->textline, data);

    // button
    sj_object_get_vector2d(data, "buttonOffset", &button->offset);
    if (!strcmp(sj_object_get_string(data, "buttonType"), "MENU")) {
        button->type = BUTTON_MENU;
        button->region = gfc_rect(button->offset.x, button->offset.y, 
                        ui_manager.menu_button->frame_w, ui_manager.menu_button->frame_h);
    }
    else {
        button->type = BUTTON_SCROLL;
        button->region = gfc_rect(button->offset.x, button->offset.y, 
                        ui_manager.scroll_button->frame_w, ui_manager.scroll_button->frame_h);
    }

    button->color = sj_object_get_color(data, "buttonColor");
}

Button* create_button(
    ButtonType b_type,
    TextLine* txt,
    GFC_Vector2D offset,
    GFC_Color color) 
{
    Button* b;
    b = gfc_allocate_array(sizeof(Button), 1);
    
    b->type = b_type;
    b->textline = *txt;
    gfc_vector2d_copy(b->offset, offset);

    switch (b->type) {
        case BUTTON_MENU:
            b->region = gfc_rect(b->offset.x, b->offset.y,
                ui_manager.menu_button->frame_w, ui_manager.menu_button->frame_h);

            break;

        case BUTTON_SCROLL:
            b->region = gfc_rect(b->offset.x, b->offset.y,
                ui_manager.scroll_button->frame_w, ui_manager.scroll_button->frame_h);

            break;
    }
    b->color = color;

    return b;
}

void create_bar_from_json(Bar* bar, SJson* data) {
    if (!bar || !data) {
        slog("couldn't create bar");
        return;
    }
    bar->sprite = gf2d_sprite_load_image(sj_object_get_string(data, "sprite"));
    sj_object_get_vector2d(data, "offset", &bar->offset);
    bar->scale = gfc_vector2d(1, 1);
}

void create_window_from_json(Window* window, SJson* data) {
    SJson* buttons, *entry;

    if (!window || !data) {
        slog("couldn't create window");
        return;
    }
    // textline
    sj_object_get_uint8(data, "hasText", &window->hasText);
    if (window->hasText) 
        create_textline_from_json(&window->textline, data);

    // window
    strcpy(window->name, sj_object_get_string(data, "name"));

    //slog("%s", window->name);
    sj_object_get_vector2d(data, "offset", &window->offset);
    sj_object_get_vector2d(data, "scale", &window->scale);
    window->color = sj_object_get_color(data, "color");

    window->ttl_then = 0;
    window->ttl_counter = 0;
    sj_object_get_uint8(data, "toggled", &window->toggled);
    if (!strncmp(sj_object_get_string(data, "type"), "NOTIF", 5)) {
        window->type = WINDOW_NOTIF;
        sj_object_get_int(data, "ttl", &window->ttl);
    }
    else {
        window->type = WINDOW_BLOCK;
        window->ttl = -1;
    }

    buttons = sj_object_get_value(data, "buttons");
    if (buttons->v.array->count) {
        window->button_count = buttons->v.array->count;
        window->wbd = gfc_allocate_array(sizeof(WindowButtonData), window->button_count);
        for (int i = 0; i < window->button_count; i++) {
            entry = sj_array_nth(buttons, i);
            window->wbd[i].drawn = 0;

            strcpy(window->wbd[i].button_name,
                sj_get_string_value(sj_array_nth(entry, 0))
            );

            sj_value_as_vector2d(sj_array_nth(entry, 1),
                &window->wbd[i].button_offset
            );

            // TODO: CHANGE LATER
            if (!strcmp(sj_get_string_value(sj_array_nth(entry, 2)), "NEXT_PREV_PAGE"))
                window->wbd[i].effect = inc_level_preview_pg;
            else if (!strcmp(sj_get_string_value(sj_array_nth(entry, 2)), "PREV_PREV_PAGE"))
                window->wbd[i].effect = dec_level_preview_pg;
            else if (!strcmp(sj_get_string_value(sj_array_nth(entry, 2)), "NEXT_ENT_LIST"))
                window->wbd[i].effect = level_editor_next_list_type;
            else if (!strcmp(sj_get_string_value(sj_array_nth(entry, 2)), "PREV_ENT_LIST"))
                window->wbd[i].effect = level_editor_prev_list_type;
            else if (!strcmp(sj_get_string_value(sj_array_nth(entry, 2)), "NEXT_ENT"))
                window->wbd[i].effect = level_editor_next_ent;
            else if (!strcmp(sj_get_string_value(sj_array_nth(entry, 2)), "PREV_ENT"))
                window->wbd[i].effect = level_editor_prev_ent;
            else if (!strcmp(sj_get_string_value(sj_array_nth(entry, 2)), "ROOM_IINC"))
                window->wbd[i].effect = level_editor_inc_room_index;
            else if (!strcmp(sj_get_string_value(sj_array_nth(entry, 2)), "ROOM_IDEC"))
                window->wbd[i].effect = level_editor_dec_room_index;
            else if (!strcmp(sj_get_string_value(sj_array_nth(entry, 2)), "ROOM_TR_INC"))
                window->wbd[i].effect = level_editor_inc_room_tr_index;
            else if (!strcmp(sj_get_string_value(sj_array_nth(entry, 2)), "ROOM_TR_DEC"))
                window->wbd[i].effect = level_editor_dec_room_tr_index;
            else
                window->wbd[i].effect = NULL;
        }
    }
    else window->wbd = NULL;
}

TextLine* create_textline(
    GFC_TextLine text,
    FontType type,
    FontSize size,
    GFC_Color color,
    GFC_Vector2D offset) 
{
    TextLine* t;
    t = gfc_allocate_array(sizeof(TextLine), 1);

    strcpy(t->text, text);
    t->type = type;
    t->size = size;
    t->color = color;
    gfc_vector2d_copy(t->offset, offset);

    return t;
}

void create_textline_from_json(TextLine* txt, SJson* data) {
    strcpy(txt->text, sj_object_get_string(data, "text"));
    //slog("%s", txt->text);
    
    if (!strncmp(sj_object_get_string(data, "fontType"), "HEADING", 7)) {
        txt->type = FONT_HEADING;
        txt->size = FONT_SIZE_HEADING;
    }
    else {
        txt->type = FONT_STANDARD;
        if (!strncmp(sj_object_get_string(data, "fontSize"), "SMALL", 5))
            txt->size = FONT_SIZE_SMALL;
        else if (!strncmp(sj_object_get_string(data, "fontSize"), "MEDIUM", 6))
            txt->size = FONT_SIZE_MEDIUM;
        else
            txt->size = FONT_SIZE_LARGE;
    }
    txt->color = sj_object_get_color(data, "textColor");
    sj_object_get_vector2d(data, "textOffset", &txt->offset);
}

void delete_menu(Menu* menu) {
    int i;

    gf2d_sprite_delete(menu->bg_sprite);
    if (menu->buttonMax) 
        free(menu->buttonList);
    
    if (menu->barMax) {
        for (i = 0; i < menu->barMax; i++) {
            gf2d_sprite_delete(menu->barList[i].sprite);
        }
        free(menu->barList);
    }
    if (menu->windowMax) {
        for (i = 0; i < menu->windowMax; i++) {
            if (menu->windowList[i].button_count)
                free(menu->windowList[i].wbd);
        }
        free(menu->windowList);
    }
    
    free(menu);
    //slog("deleted menu");
}

void toggle_window(const char* name, Uint8 toggle) {
    Menu* menu;
    Window* win;
    int i;
    
    // find window
    menu = (Menu*) gfc_list_nth(ui_manager.ui_list, get_world_state());
    win = NULL;
    for (i = 0; i < menu->windowMax; i++) {
        win = &menu->windowList[i];
        if (!win || win->type == WINDOW_BLOCK) continue;

        if (!strcmp(win->name, name))
            break;
        else win = NULL;
    }

    // assign toggle
    if (win)
        win->toggled = toggle;
    else slog("couldn't find window %s", name);
}

void draw_window(Window* win) {
    GFC_Rect region;
    GFC_Vector2D offset;
    Uint8 center;
    float padding = 10.0f;

    if (!win) return;

    gf2d_sprite_draw(ui_manager.window, win->offset,
                     &win->scale, NULL, NULL, NULL, &win->color, 0);

    if (!win->hasText) return;

    region = gfc_rect(win->offset.x + padding, win->offset.y, win->scale.x - (padding * 2.0f), win->scale.y);
    center = win->textline.offset.x == -1.0f && win->textline.offset.y == -1.0f ? 1 : 0;
    if (!center) { // not centered
        offset = gfc_vector2d(region.x + win->textline.offset.x,
                                region.y + win->textline.offset.y);

        font_display_text(
            win->textline.text,
            win->textline.type,
            win->textline.size,
            win->textline.color,
            0,
            &offset,
            NULL
        );
    }
    else {
        font_display_text(
            win->textline.text,
            win->textline.type,
            win->textline.size,
            win->textline.color,
            1,
            NULL,
            &region
        );
    }
}

void draw_notif_windows(Menu* m) {
    Window* win;

    for (int i = 0; i < m->windowMax; i++) {
        win = &m->windowList[i];
        if (win->type == WINDOW_BLOCK || !win->toggled) continue;

        draw_window(win);
        window_ttl_advance(win);
    }
}

void draw_button(Button* button, Uint8 index) {
    Sprite* b = NULL;
    GFC_Color color;

    if (!button) return;

    // draw button (change color if currently selected_
    if (index == ui_manager.active_button)
        color = gfc_color8(120, 120, 120, 255);
    else color = button->color;

    switch (button->type){
        case BUTTON_MENU:
            b = ui_manager.menu_button;
            break;
        case BUTTON_SCROLL:
            b = ui_manager.scroll_button;
            break;
    }

    gf2d_sprite_draw(b, button->offset, 
                     NULL, NULL, NULL, NULL, &color, 0);

    // draw text on it
    font_display_text(
        button->textline.text, 
        button->textline.type, 
        button->textline.size, 
        button->textline.color,
        1,
        NULL,
        &button->region
    );
}

void window_ttl_advance(Window* win) {
    if (checkFramePass(win->ttl_then)) {
        win->ttl_then = CURRENT_TIME;
        win->ttl_counter++;
    }

    if (win->ttl_counter > win->ttl) {
        win->toggled = 0;
        win->ttl_counter = 0;
    }
}

void reset_window_toggles() {
    Menu* menu;
    Window* win;
    int i;

    // find window
    menu = (Menu*) gfc_list_nth(ui_manager.ui_list, get_world_state());
    for (i = 0; i < menu->windowMax; i++) {
        win = &menu->windowList[i];
        if (!win) continue;

        win->toggled = 0;
        win->ttl_counter = 0;
        win->ttl_then = 0;
    }
}

void reset_window_wbds(Window* win) {
    if (!win) return;
    for (int i = 0; i < win->button_count; i++) {
        win->wbd[i].drawn = 0;
    }
}

Uint32 find_button_index_from_wbd(Menu* menu, WindowButtonData* wbd) {
    Button* button;
    Uint32 i;

    for (i = 0; i < menu->buttonMax; i++) {
        button = &menu->buttonList[i];
        if (!button) continue;

        if (!strcmp(button->textline.text, wbd->button_name) && !wbd->drawn) {
            wbd->drawn = 1;
            return i;
        }
    }
    return menu->buttonMax + 1;
}

Window* get_window_by_name(Window* list, Uint32 count, const char* name) {
    Window* win;

    if (!list) return NULL;

    for (int i = 0; i < count; i++) {
        win = &list[i];
        if (!win) continue;

        if (!gfc_word_cmp(win->name, name))
            return win;
    }
    return NULL;
}

void draw_player_hud(Menu* menu, GFC_List* enemy_list, PlayerData* p_data) {
    Entity* enemy;
    EnemyData* e_data;
    Bar* bar;
    Level* l;
    GFC_Rect rect;
    int i, j;

    if (!enemy_list) {
        slog("no enemy list");
        return;
    }

    if (!p_data) {
        slog("no player data provided");
        return;
    }

    // draw enemy and player health bars
    for (i = 0; i < menu->barMax; i++) {
        bar = &menu->barList[i];
        if (i == 0) { // draw player hud bars
            bar->scale = gfc_vector2d(p_data->currHealth / p_data->maxHealth, 1);
            gf2d_sprite_draw(bar->sprite, bar->offset, &bar->scale, NULL, NULL, NULL, NULL, 0);
        }
        else if (i == 1) {
            if (!enemy_list->count) {
                //slog("no more enemies");
                continue;
            }

            for (j = 0; j < enemy_list->count; j++) { // draw enemy hud bars
                enemy = (Entity*)gfc_list_nth(enemy_list, j);
                if (!enemy) {
                    slog("no enemy");
                    continue;
                }

                e_data = (EnemyData*)enemy->data;
                if (!e_data) {
                    slog("no enemy data");
                    continue;
                }

                bar->scale = gfc_vector2d(e_data->currHealth / e_data->maxHealth, 1);
                bar->offset = gfc_vector2d(enemy->position.x - bar->sprite->frame_w * 0.5f,
                    enemy->position.y - enemy->sprite->frame_h);
                gf2d_sprite_draw(bar->sprite, bar->offset, &bar->scale, NULL, NULL, NULL, NULL, 0);
            }
        }
        else {
            // draw other bars
        }
    }

    // display level name
    l = get_curr_level();
    rect = gfc_rect(1130, 10, 150, 90);

    gf2d_draw_rect_filled(rect, gfc_color8(120, 0, 0, 120));
    font_display_text(
        l->name,
        FONT_STANDARD,
        FONT_SIZE_MEDIUM,
        GFC_COLOR_WHITE,
        1,
        NULL,
        &rect
    );
}

void draw_world_menu(Menu* menu, WorldState world_state) {
    Window* win;
    Button* button;
    WindowButtonData* wbd;
    int i, j;

    gf2d_sprite_draw_image(menu->bg_sprite, gfc_vector2d(0, 0)); // draw bg
    menu_check_input(menu, 0, menu->buttonMax - 1, NULL);

    if (world_state == WORLD_LEVEL_SELECT) {
        mouse_update_state();

        win = &menu->windowList[0];
        if (!win) {
            change_world_state(WORLD_MAINMENU);
            slog("no window found");
            return;
        }

        draw_window(win);
        for (i = 0; i < win->button_count; i++) {
            wbd = &win->wbd[i];
            if (!wbd) continue;

            j = find_button_index_from_wbd(menu, wbd);

            // if button was found, draw it, and handle input
            if (j < menu->buttonMax) {
                // draw button
                button = &menu->buttonList[j];
                update_button_offset(button, wbd->button_offset);

                if (mouse_in_rect(button->region)) {
                    ui_manager.active_button = j;
                    if (mouse_button_pressed(MOUSE_LEFT_CLICK))
                        button->selected = 1;
                }
                else ui_manager.active_button = -1;

                draw_button(button, j);

                // handle button inputs
                if (button->selected) {
                    button->selected = 0;
                    reset_window_toggles();

                    if (!strcmp(button->textline.text, "GO BACK")) {
                        ui_manager.active_button = 0;
                        free_level_previews();
                        change_world_state(WORLD_MAINMENU);
                        return;
                    }

                    if (wbd->effect) wbd->effect();
                }
            }
        }

        draw_level_previews();
        draw_notif_windows(menu);
        draw_mouse();

        reset_window_wbds(win);
    }
    else { // draw as normal
        // draw BLOCK windows
        for (i = 0; i < menu->windowMax; i++) {
            win = &menu->windowList[i];
            if (win->type == WINDOW_NOTIF) continue;

            draw_window(win);
        }

        // draw buttons
        for (i = 0; i < menu->buttonMax; i++) {
            button = &menu->buttonList[i];
            draw_button(button, i);

            // check if button has been selected
            if (button && button->selected) {
                button->selected = 0;
                ui_manager.active_button = 0;
                reset_window_toggles();

                switch (world_state) {
                    case WORLD_MAINMENU:
                        if (!strncmp(button->textline.text, "PLAY", 4)) {
                            level_manager_init("config/levels.cfg");
                            initialize_level_previews();
                            change_world_state(WORLD_LEVEL_SELECT);
                            return;
                        }
                        else if (!strncmp(button->textline.text, "LEVEL EDITOR", 12))
                            change_world_state(WORLD_EDITOR_START);
                        else if (!strncmp(button->textline.text, "QUIT", 4))
                            change_world_state(WORLD_CLOSE);

                        break;

                    case WORLD_PAUSEMENU:
                        if (!strncmp(button->textline.text, "RESUME", 6))
                            change_world_state(WORLD_INGAME);
                        else if (!strncmp(button->textline.text, "QUIT", 4))
                            change_world_state(WORLD_GAMEPLAY_CLOSE);

                        break;

                    case WORLD_LEVELCOMPLETE:
                        if (!strncmp(button->textline.text, "NEXT", 4)) // load next level here
                            change_world_state(WORLD_LOAD_NEXT_LEVEL);
                        else if (!strncmp(button->textline.text, "QUIT", 4))
                            change_world_state(WORLD_GAMEPLAY_CLOSE);

                        break;

                    case WORLD_PLAYERDEAD:
                        if (!strncmp(button->textline.text, "RESPAWN", 7)) // load next level here
                            change_world_state(WORLD_RESTART_LEVEL);
                        else if (!strncmp(button->textline.text, "QUIT", 4))
                            change_world_state(WORLD_GAMEPLAY_CLOSE);

                        break;

                    case WORLD_GAME_COMPLETE:
                        if (!strncmp(button->textline.text, "QUIT", 4))
                            change_world_state(WORLD_GAMEPLAY_CLOSE);

                        break;
                }
            }
        }

        // draw NOTIF windows
        draw_notif_windows(menu);
    }

    //draw_mouse();
}

void draw_editor_menu(Menu* menu, EditorState editor_state) {
    Window* win;
    WindowButtonData* wbd = NULL;
    Button* button = NULL;
    int i, j;

    gf2d_sprite_draw_image(menu->bg_sprite, gfc_vector2d(0, 0)); // draw bg

    i = (int) get_level_editor_state();
    win = &menu->windowList[i];

    draw_window(win);
    if (!win->button_count) return;

    // draw buttons
    for (i = 0; i < win->button_count; i++) {
        wbd = &win->wbd[i];
        if (!wbd) continue;

        j = find_button_index_from_wbd(menu, wbd);

        // if button was found, draw it and handle input
        if (j < menu->buttonMax) {
            // draw button
            button = &menu->buttonList[j];
            update_button_offset(button, wbd->button_offset);

            if (mouse_in_rect(button->region)) {
                ui_manager.active_button = j;
                if (mouse_button_pressed(MOUSE_LEFT_CLICK))
                    button->selected = 1;
            }
            else ui_manager.active_button = -1;

            draw_button(button, j);

            // handle button inputs
            if (button->selected) {
                button->selected = 0;
                ui_manager.active_button = 0;
                reset_window_toggles();

                switch (editor_state) {
                    case EDITOR_SELECT_MODE:
                        if (!strncmp(button->textline.text, "QUIT", 4)) {
                            change_world_state(WORLD_EDITOR_CLOSE);
                            return;
                        }
                        else if (!strcmp(button->textline.text, "NEW LEVEL")) {
                            initialize_dummy_level();
                            initialize_level_editor_all_entities();
                            set_level_editor_state(EDITOR_EDITING);
                            return;
                        }
                        else if (!strcmp(button->textline.text, "LOAD EXISTING LEVEL")) {
                            initialize_level_previews();
                            set_level_editor_state(EDITOR_EXISTING_LEVELS);
                            return;
                        }

                        break;

                    case EDITOR_EXISTING_LEVELS:
                        if (!strcmp(button->textline.text, "QUIT")) {
                            free_level_previews();
                            change_world_state(WORLD_EDITOR_CLOSE);
                            return;
                        }
                        else if (!strcmp(button->textline.text, "GO BACK")) {
                            free_level_previews();
                            set_level_editor_state(EDITOR_SELECT_MODE);
                            return;
                        }

                        break;
                }

                if (wbd->effect) wbd->effect();
            }
        }
    }

    // draw other UI elements
    if (editor_state == EDITOR_EXISTING_LEVELS){
        // draw level buttons
        draw_level_previews();
    }

    draw_notif_windows(menu);
    draw_mouse();

    reset_window_wbds(win);
}

void draw_editor_hud_buttons(Menu* menu, Window* win) {
    EditorDrawMode drawMode;
    EditorTrnMode trnMode;
    WindowButtonData* wbd;
    Button* button;
    int i, j;

    drawMode = get_level_editor_draw_mode();
    trnMode = get_level_editor_trn_mode();
    for (i = 0; i < win->button_count; i++) {
        wbd = &win->wbd[i];
        if (!wbd) continue;

        j = find_button_index_from_wbd(menu, wbd);

        // if button was found, draw it and handle input
        if (j < menu->buttonMax) {
            // draw button
            button = &menu->buttonList[j];
            update_button_offset(button, wbd->button_offset);

            if (mouse_in_rect(button->region)) {
                ui_manager.active_button = j;
                if (mouse_button_pressed(MOUSE_LEFT_CLICK))
                    button->selected = 1;
            }
            else ui_manager.active_button = -1;

            if ((!strcmp(button->textline.text, "GND") && trnMode == EDITOR_TRN_GROUND)
                || (!strcmp(button->textline.text, "PLT") && trnMode == EDITOR_TRN_PLAT))
            {
                ui_manager.active_button = j;
            }

            draw_button(button, j);

            // handle button inputs
            if (button->selected) {
                button->selected = 0;
                ui_manager.active_button = 0;
                reset_window_toggles();

                if (!strcmp(button->textline.text, "QUIT")) {
                    change_world_state(WORLD_EDITOR_CLOSE);
                    return;
                }
                else if (!strcmp(button->textline.text, "SAVE")) {
                    level_editor_save_new_level();
                    return;
                }

                switch (drawMode) {
                    case EDITOR_DRAW_ENTITY:
                        if (!strcmp(button->textline.text, "P-SPWN")) {
                            // spawn player
                            level_editor_set_player_spawn();
                        }

                        break;

                    case EDITOR_DRAW_TERRAIN:
                        if (!strcmp(button->textline.text, "GND")) 
                            set_level_editor_trn_mode(EDITOR_TRN_GROUND);
                        else if (!strcmp(button->textline.text, "PLT"))
                            set_level_editor_trn_mode(EDITOR_TRN_PLAT);
                        else if (!strcmp(button->textline.text, "TRANSITIONS") && level_editor_room_transition_enough_rooms()) {
                            // load menu for making room transitions
                            set_level_editor_state(EDITOR_TRANSITIONS_MENU);
                        }
                }

                if (wbd->effect) wbd->effect();
            }
        }
    }
    reset_window_wbds(win);
}

void draw_editor_hud(Menu* menu) {
    Window* win;
    GFC_TextWord* strings;
    EditorRoom* e_room;
    GFC_Rect name_rect;
    int i;

    if (get_level_editor_hud_toggle()) { // draw all windows if hud is toggled
        if (get_level_editor_draw_mode() == EDITOR_DRAW_TERRAIN) {
            win = get_window_by_name(menu->windowList, menu->windowMax, "TRN_SIDE_MENU");
            if (!win) return;

            draw_window(win);

            // draw buttons
            draw_editor_hud_buttons(menu, win);
        }
        else { // ENTITY_DRAW_ENTITY
            for (i = 0; i < 2; i++) {
                win = &menu->windowList[i];
                if (!win) continue;

                // modify text for last window (current entity type)
                if (i == 1) {
                    strings = get_level_editor_ent_strings();
                    strcpy(win->textline.text, strings[get_level_editor_list_type() - 1]);
                }

                draw_window(win);
                if (!win->button_count) continue;

                // draw buttons
                draw_editor_hud_buttons(menu, win);
            } 

            // TODO: draw other elements
            level_editor_draw_entity_preview_region();
        }

        if (get_level_editor_show_controls()) {
            win = get_window_by_name(menu->windowList, menu->windowMax, "CONTROLS");
            draw_window(win);
        }

        draw_notif_windows(menu);
    }

    // display room name
    e_room = get_current_editor_room();
    name_rect = gfc_rect(2, 620, 400, 100);
    if (e_room) {
        font_display_text(
            e_room->name,
            FONT_HEADING,
            FONT_SIZE_HEADING,
            gfc_color8(255, 255, 255, 180),
            1,
            NULL,
            &name_rect
        );
    }
    draw_mouse();
}

void draw_editor_hud_special_menu(Menu* menu) {
    EditorState editor_state = get_level_editor_state();
    Window* win = NULL;
    WindowButtonData* wbd;
    Button* button;
    GFC_Vector2D room_in_pos;
    int i, j;

    if (editor_state == EDITOR_ROOMS_MENU) {
        win = get_window_by_name(menu->windowList, menu->windowMax, "ROOM_PROP");
        room_in_pos = gfc_vector2d(550, 250);
    }
    else if (editor_state == EDITOR_TRANSITIONS_MENU) {
        win = get_window_by_name(menu->windowList, menu->windowMax, "RM_TRANS");
        room_in_pos = gfc_vector2d(350, 300);
    }

    if (!win) return;
    draw_window(win);

    // draw current room name
    font_display_text(
        get_current_editor_room()->name,
        FONT_STANDARD,
        FONT_SIZE_MEDIUM,
        GFC_COLOR_WHITE,
        0,
        &room_in_pos,
        NULL
    );

    // draw other room names for transition menu
    if (editor_state == EDITOR_TRANSITIONS_MENU)
        level_editor_display_room_transition_name();
   
    // draw buttons
    for (i = 0; i < win->button_count; i++) {
        wbd = &win->wbd[i];
        if (!wbd) continue;

        j = find_button_index_from_wbd(menu, wbd);

        // if button was found, draw it and handle input
        if (j < menu->buttonMax) {
            button = &menu->buttonList[j];
            update_button_offset(button, wbd->button_offset);

            if (mouse_in_rect(button->region)) {
                ui_manager.active_button = j;
                if (mouse_button_pressed(MOUSE_LEFT_CLICK))
                    button->selected = 1;
            }
            else ui_manager.active_button = -1;

            draw_button(button, j);

            // handle button inputs
            if (button->selected) {
                button->selected = 0;
                ui_manager.active_button = 0;
                reset_window_toggles();

                switch (editor_state) {
                    case EDITOR_ROOMS_MENU:
                        if (!strcmp(button->textline.text, "ADD ROOM"))
                            level_editor_create_new_room();
                        else if (!strcmp(button->textline.text, "RMV ROOM"))
                            level_editor_remove_room(); 
                    
                    break;

                    case EDITOR_TRANSITIONS_MENU:
                        if (!strcmp(button->textline.text, "QUIT")) {
                            set_level_editor_state(EDITOR_EDITING);
                            return;
                        }
                        else if (!strcmp(button->textline.text, "SAVE")) {
                            // create or change room transition for both rooms
                            level_editor_create_room_transition();
                            set_level_editor_state(EDITOR_EDITING);
                            return;
                        }
                      
                    break;
                }

                if (wbd->effect) wbd->effect();
            }
        }
    }

    reset_window_wbds(win);
    draw_notif_windows(menu);
    draw_mouse();
}

void drawUI(Uint8 w_state) {    
    Menu* menu;
    WorldState world_state = (WorldState) w_state;
    EditorState editor_state = get_level_editor_state();

    menu = (Menu*)gfc_list_nth(ui_manager.ui_list, w_state);
    if (!menu) return;

    if (world_state == WORLD_EDITOR) {
        mouse_update_state();

        if (editor_state == EDITOR_EDITING) {
            menu = (Menu*)gfc_list_nth(ui_manager.ui_list, w_state + 1);
            if (!menu) return;
            draw_editor_hud(menu);
        }
        else if (editor_state == EDITOR_ROOMS_MENU || editor_state == EDITOR_TRANSITIONS_MENU) {
            menu = (Menu*)gfc_list_nth(ui_manager.ui_list, w_state + 1);
            if (!menu) return;
            draw_editor_hud_special_menu(menu);
        }
        else draw_editor_menu(menu, editor_state);
    }
    else if (world_state != WORLD_INGAME) 
        draw_world_menu(menu, world_state);
    else 
        draw_player_hud(menu, get_enemy_list(), get_player_data());
}

void menu_buttonList_advance(int min, int max, Window* win) {
    if (win) {
        /*
        if (ui_manager.button_range_index < win->button_ranges_count - 1)
            ++ui_manager.button_range_index;
        else
            ui_manager.button_range_index = 0;
            
        ui_manager.active_button = win->button_ranges[ui_manager.button_range_index];
        */
    }
    else {
        if (ui_manager.active_button < max)
            ui_manager.active_button++;
        else
            ui_manager.active_button = min;
    }
}

void menu_buttonList_go_back(int min, int max, Window* win) {
    if (win) {
        /*
        if (ui_manager.button_range_index) 
            --ui_manager.button_range_index;
        else
            ui_manager.button_range_index = win->button_ranges_count - 1;
  
        ui_manager.active_button = win->button_ranges[ui_manager.button_range_index];
        */
    }
    else {
        if (ui_manager.active_button)
            ui_manager.active_button--;
        else
            ui_manager.active_button = max;
    }
}

void menu_check_input(Menu* menu, int min, int max, Window* win) {
    Button* active_button;

    if (!menu) {
        slog("menu is null");
        return;
    }

    // menu traversal
    switch (menu->button_layout) {
        case MENU_BUTTON_HORIZONTAL:
            if (gfc_input_command_pressed("moveright"))
                menu_buttonList_advance(min, max, win);
            else if (gfc_input_command_pressed("moveleft"))
                menu_buttonList_go_back(min, max, win);

            break;

        case MENU_BUTTON_VERTICAL:
            if (gfc_input_command_pressed("moveup"))
                menu_buttonList_go_back(min, max, win);
            else if (gfc_input_command_pressed("movedown"))
                menu_buttonList_advance(min, max, win);

            break;

        /*
        case MENU_BUTTON_CARDINAL: //TODO: fix later
            if (gfc_input_command_pressed("moveup")) {
                if (ui_manager.active_button)
                    ui_manager.active_button--;
            }
            else if (gfc_input_command_pressed("movedown")) {
                if (ui_manager.active_button < max)
                    ui_manager.active_button++;
            }
            else if (gfc_input_command_pressed("moveright")) {
                if (ui_manager.active_button < max)
                    ui_manager.active_button++;
            }
            else if (gfc_input_command_pressed("moveleft")) {
                if (ui_manager.active_button)
                    ui_manager.active_button--;
            }

            break;
        */
    }

    // button selection
    if (gfc_input_command_pressed("interact")) {
        active_button = &menu->buttonList[ui_manager.active_button];
        active_button->selected = 1;
    }
}

void update_button_offset(Button* b, GFC_Vector2D new) {
    gfc_vector2d_copy(b->offset, new);
    b->region.x = new.x;
    b->region.y = new.y;
}
#include "simple_logger.h"
#include "gfc_input.h"
#include "gf2d_draw.h"
#include "world.h"
#include "ui.h"
#include "player.h"
#include "enemy.h"

typedef struct UIManager_S {
    GFC_List*       ui_list;
    int             active_button;
    Sprite*         menu_block;
}UIManager;

static UIManager ui_manager = { 0 };

void ui_system_close();
Menu* create_menu(char* filename);
void create_button(Button* button, SJson* data);
void create_bar(Bar* bar, SJson* data);
void delete_menu(Menu* win);
void draw_button(Button* button, Uint8 index);
void menu_check_input(Menu* win);

void ui_system_init(char* configFile) {
    SJson* config, *menu_list;
    char* path;
    int i;
    
    // initialize all menus
    config = sj_load(configFile);
    if (!config) return;

    ui_manager.ui_list = gfc_list_new();
    ui_manager.menu_block = gf2d_sprite_load_image(sj_object_get_string(config, "button"));

    menu_list = sj_object_get_value(config, "menu_list");
    for (i = 0; i < menu_list->v.array->count; i++) {
        path = sj_object_get_string(sj_array_get_nth(menu_list, i), "path");
        gfc_list_append(ui_manager.ui_list, create_menu(path));
    }

    ui_manager.active_button = 0;
    sj_free(config);
    atexit(ui_system_close);
}

void ui_system_close() {
    gfc_list_foreach(ui_manager.ui_list, delete_menu);
    gfc_list_delete(ui_manager.ui_list);

    gf2d_sprite_delete(ui_manager.menu_block);

    memset(&ui_manager, 0, sizeof(UIManager));
}

Menu* create_menu(char* filename) {
    SJson* data, *win_data;
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
    sj_object_get_vector2d(win_data, "offset", &menu->offset);

    strcpy(buffer, sj_object_get_string(win_data, "bg_sprite"));
    if (strcmp(buffer, "no sprite")) {
        menu->bg_sprite = gf2d_sprite_load_image(buffer);
        menu->dimensions = gfc_rect(menu->offset.x, menu->offset.y, menu->bg_sprite->frame_w, menu->bg_sprite->frame_h);
        //slog("menu sprite initialized");
    }
    //else slog("no sprite provided");

    // create buttons
    sj_object_get_uint8(win_data, "buttonMax", &menu->buttonMax);
    if (menu->buttonMax) {
        menu->buttonList = gfc_allocate_array(sizeof(Button), menu->buttonMax);

        sj_object_get_int(win_data, "buttonLayout", &i);    
        menu->button_layout = (ButtonLayout) i;

        win_data = sj_object_get_value(data, "buttons");
        for (i = 0; i < menu->buttonMax; i++) {
            create_button(&menu->buttonList[i], sj_array_get_nth(win_data, i));
        }
    }

    // create bars
    sj_object_get_uint8(win_data, "barMax", &menu->barMax);
    if (menu->barMax) {
        menu->barList = gfc_allocate_array(sizeof(Bar), menu->barMax);
        win_data = sj_object_get_value(data, "bars");
        for (i = 0; i < menu->barMax; i++) {
            create_bar(&menu->barList[i], sj_array_get_nth(win_data, i));
        }
    }

    sj_free(data);
    return menu;
}

void create_button(Button* button, SJson* data) {
    if (!button || !data) {
        slog("couldn't create button");
        return;
    }

    // textline
    strcpy(button->textline.text, sj_object_get_string(data, "text"));

    if (!strncmp(sj_object_get_string(data, "fontType"), "HEADING", 7)) {
        button->textline.type = FONT_HEADING;
        button->textline.size = FONT_SIZE_HEADING;
    }
    else {
        button->textline.type = FONT_STANDARD;
        if (!strncmp(sj_object_get_string(data, "fontSize"), "SMALL", 5))
            button->textline.size = FONT_SIZE_SMALL;
        else if (!strncmp(sj_object_get_string(data, "fontSize"), "MEDIUM", 6))
            button->textline.size = FONT_SIZE_MEDIUM;
        else
            button->textline.size = FONT_SIZE_LARGE;
    }
    button->textline.color = sj_object_get_color(data, "textColor");

    // button
    sj_object_get_vector2d(data, "buttonOffset", &button->offset);
    button->region = gfc_rect(button->offset.x, button->offset.y, ui_manager.menu_block->frame_w, ui_manager.menu_block->frame_h);
    button->color = sj_object_get_color(data, "buttonColor");
}

void create_bar(Bar* bar, SJson* data) {
    if (!bar || !data) {
        slog("couldn't create bar");
        return;
    }
    bar->sprite = gf2d_sprite_load_image(sj_object_get_string(data, "sprite"));
    sj_object_get_vector2d(data, "offset", &bar->offset);
    bar->scale = gfc_vector2d(1, 1);
}

void delete_menu(Menu* menu) {
    int i;

    gf2d_sprite_delete(menu->bg_sprite);
    if (menu->buttonMax) {
        free(menu->buttonList);
    }
    if (menu->barMax) {
        for (i = 0; i < menu->barMax; i++) {
            gf2d_sprite_delete(menu->barList[i].sprite);
        }
        free(menu->barList);
    }
    free(menu);
    slog("deleted menu");
}

void draw_button(Button* button, Uint8 index) {
    GFC_Vector2D text_offset;
    GFC_Color color;

    if (!button) return;

    // draw button (change color if currently selected
    if (index == ui_manager.active_button)
        color = gfc_color8(0, 60, 120, 255);
    else color = button->color;

    gf2d_sprite_draw(ui_manager.menu_block, button->offset, 
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

void drawUI(Uint8 w_state) {
    WorldState world_state;
    PlayerData* p_data;
    EnemyData* e_data;
    GFC_List* enemy_list;
    Entity* enemy;
    Menu* menu;
    Button* button;
    Bar* bar;
    int i, j;

    world_state = (WorldState) w_state;

    menu = (Menu*) gfc_list_nth(ui_manager.ui_list, w_state);
    if (world_state != WORLD_INGAME) {
        gf2d_sprite_draw_image(menu->bg_sprite, menu->offset); // draw bg
        menu_check_input(menu);
        
        // draw buttons
        for (i = 0; i < menu->buttonMax; i++) {
            button = &menu->buttonList[i];

            draw_button(button, i);

            // check if button has been selected
            if (button->selected) {
                button->selected = 0;
                switch (world_state) {
                    case WORLD_MAINMENU:
                        if (!strncmp(button->textline.text, "PLAY", 4))
                            change_world_state(WORLD_GAMESTART);
                        else if (!strncmp(button->textline.text, "QUIT", 4))
                            change_world_state(WORLD_CLOSE);

                        break;

                    case WORLD_PAUSEMENU:
                        if (!strncmp(button->textline.text, "RESUME", 6))
                            change_world_state(WORLD_INGAME);
                        else if (!strncmp(button->textline.text, "QUIT", 4))
                            change_world_state(WORLD_GAMECLOSE);

                        break;

                    case WORLD_LEVELCOMPLETE:
                        if (!strncmp(button->textline.text, "NEXT", 4)) // load next level here
                            change_world_state(WORLD_LOAD_NEXT_LEVEL);
                        else if (!strncmp(button->textline.text, "QUIT", 4))
                            change_world_state(WORLD_GAMECLOSE);

                        break;

                    case WORLD_PLAYERDEAD:
                        if (!strncmp(button->textline.text, "RESPAWN", 7)) // load next level here
                            change_world_state(WORLD_RESTART_LEVEL);
                        else if (!strncmp(button->textline.text, "QUIT", 4))
                            change_world_state(WORLD_GAMECLOSE);

                        break;

                    case WORLD_GAME_COMPLETE:
                        if (!strncmp(button->textline.text, "QUIT", 4))
                            change_world_state(WORLD_GAMECLOSE);

                        break;
                }
            }   
        }
    }
    else {
        enemy_list = get_enemy_list();
        if (!enemy_list) {
            slog("no enemy list");
            return;
        }

        p_data = (PlayerData*)get_player_data();
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
    }
}

void menu_check_input(Menu* menu) {
    Button* active_button;

    if (!menu) {
        slog("menu is null");
        return;
    }

    // menu traversal
    switch (menu->button_layout) {
        case MENU_BUTTON_HORIZONTAL:
            if (gfc_input_command_pressed("moveright")) {
                if (ui_manager.active_button < menu->buttonMax - 1)
                    ui_manager.active_button++;
            }
            else if (gfc_input_command_pressed("moveleft")) {
                if (ui_manager.active_button)
                    ui_manager.active_button--;
            }

            break;

        case MENU_BUTTON_VERTICAL:
            if (gfc_input_command_pressed("moveup")) {
                if (ui_manager.active_button) 
                    ui_manager.active_button--;
            }
            else if (gfc_input_command_pressed("movedown")) {
                if (ui_manager.active_button < menu->buttonMax - 1)
                    ui_manager.active_button++;
            }

            break;

        case MENU_BUTTON_CARDINAL: //TODO: fix later
            if (gfc_input_command_pressed("moveup")) {
                if (ui_manager.active_button)
                    ui_manager.active_button--;
            }
            else if (gfc_input_command_pressed("movedown")) {
                if (ui_manager.active_button < menu->buttonMax - 1)
                    ui_manager.active_button++;
            }
            else if (gfc_input_command_pressed("moveright")) {
                if (ui_manager.active_button < menu->buttonMax - 1)
                    ui_manager.active_button++;
            }
            else if (gfc_input_command_pressed("moveleft")) {
                if (ui_manager.active_button)
                    ui_manager.active_button--;
            }

            break;
    }

    // button selection
    if (gfc_input_command_pressed("interact")) {
        active_button = &menu->buttonList[ui_manager.active_button];
        active_button->selected = 1;
    }
}
#include "simple_logger.h"
#include "gfc_input.h"
#include "world.h"
#include "ui.h"

typedef struct UIManager_S {
    GFC_List*       ui_list;
    int             active_button;
    GFC_Color       active_shift;
}UIManager;

static UIManager ui_manager = { 0 };

void ui_system_close();
Menu* create_menu(char* filename);
void create_button(Button* button, SJson* data);
void delete_menu(Menu* menu);
void menu_check_input(Menu* menu);

void ui_system_init(char* configFile) {
    SJson* config, *menu_list;
    char* path;
    int i;
    
    // initialize all menus
    config = sj_load(configFile);
    if (!config) return;

    ui_manager.ui_list = gfc_list_new();

    menu_list = sj_object_get_value(config, "menu_list");
    for (i = 0; i < menu_list->v.array->count; i++) {
        path = sj_object_get_string(sj_array_get_nth(menu_list, i), "path");
        gfc_list_append(ui_manager.ui_list, create_menu(path));
    }

    ui_manager.active_shift = sj_object_get_color(config, "active_shift");
    ui_manager.active_button = 0;
    sj_free(config);
    atexit(ui_system_close);
}

void ui_system_close() {
    gfc_list_foreach(ui_manager.ui_list, delete_menu);
    gfc_list_delete(ui_manager.ui_list);

    memset(&ui_manager, 0, sizeof(UIManager));
}

Menu* create_menu(char* filename) {
    SJson* data, *menu_data;
    Menu* menu;
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

    menu_data = sj_object_get_value(data, "menu");
    menu->bg_sprite = gf2d_sprite_load_image(sj_object_get_string(menu_data, "bg_sprite"));
  
    sj_object_get_vector2d(menu_data, "offset", &menu->offset);
    menu->dimensions = gfc_rect(menu->offset.x, menu->offset.y, menu->bg_sprite->frame_w, menu->bg_sprite->frame_h);

    sj_object_get_int(menu_data, "buttonLayout", &i);
    menu->button_layout = (ButtonLayout) i;

    // create buttons
    sj_object_get_uint8(menu_data, "buttonMax", &menu->buttonMax);
    menu->buttonList = gfc_allocate_array(sizeof(Button), menu->buttonMax);

    menu_data = sj_object_get_value(data, "buttons");
    for (i = 0; i < menu->buttonMax; i++) {
        create_button(&menu->buttonList[i], sj_array_get_nth(menu_data, i));
    }

    sj_free(data);
    return menu;
}

void create_button(Button* button, SJson* data) {
    if (!button || !data) {
        slog("couldn't create button");
        return;
    }
    strcpy(button->cmd, sj_object_get_string(data, "cmd"));
    button->sprite = gf2d_sprite_load_image(sj_object_get_string(data, "sprite"));
    sj_object_get_vector2d(data, "offset", &button->offset);
    button->region = gfc_rect(button->offset.x, button->offset.y, button->sprite->frame_w, button->sprite->frame_h);
}

void delete_menu(Menu* menu) {
    int i;

    gf2d_sprite_delete(menu->bg_sprite);
    for (i = 0; i < menu->buttonMax; i++) {
        gf2d_sprite_delete(menu->buttonList[i].sprite);
    }
    free(menu->buttonList);
    free(menu);
    slog("deleted menu");
}

void drawUI(Uint8 w_state) {
    WorldState world_state;
    GFC_Color* shift;
    Menu* menu;
    Button* button;
    int i;

    //SDL_PollEvent(&mouse);
    world_state = (WorldState) w_state;

    shift = NULL;
    switch (world_state) {
        case WORLD_MAINMENU:
            menu = (Menu*) gfc_list_nth(ui_manager.ui_list, 0);
            
            // draw bg
            gf2d_sprite_draw_image(menu->bg_sprite, menu->offset);

            // draw buttons
            menu_check_input(menu);
            for (i = 0; i < menu->buttonMax; i++) {
                button = &menu->buttonList[i];
                if (i == ui_manager.active_button) shift = &ui_manager.active_shift;
                gf2d_sprite_draw(button->sprite, button->offset, NULL, NULL, NULL, NULL, shift, 0);
                shift = NULL;

                // check if button has been selected
                if (button->selected) {
                    if (!strcmp(button->cmd, "PLAY"))
                        change_world_state(WORLD_GAMESTART);
                    else if (!strcmp(button->cmd, "QUIT"))
                        change_world_state(WORLD_CLOSE);

                    button->selected = 0;
                }
            }

            break;

        case WORLD_INGAME:



            break;

        case WORLD_PAUSEMENU:
            menu = (Menu*) gfc_list_nth(ui_manager.ui_list, 1);

            // draw bg
            gf2d_sprite_draw_image(menu->bg_sprite, menu->offset);

            // draw buttons
            menu_check_input(menu);
            for (i = 0; i < menu->buttonMax; i++) {
                button = &menu->buttonList[i];
                if (i == ui_manager.active_button) shift = &ui_manager.active_shift;
                gf2d_sprite_draw(button->sprite, button->offset, NULL, NULL, NULL, NULL, shift, 0);
                shift = NULL;

                // check if button has been selected
                if (button->selected) {
                    if (!strcmp(button->cmd, "RESUME"))
                        change_world_state(WORLD_INGAME);
                    else if (!strcmp(button->cmd, "QUIT"))
                        change_world_state(WORLD_GAMECLOSE);

                    button->selected = 0;
                }
            }
            
            break;

        case WORLD_PLAYERDEAD:

            break;

        case WORLD_LEVELCOMPLETE:

            break;
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
    if (gfc_input_command_pressed("attack")) {
        active_button = &menu->buttonList[ui_manager.active_button];
        active_button->selected = 1;
    }
}
#include "simple_logger.h"
#include "gfc_input.h"
#include "world.h"
#include "ui.h"
#include "player.h"
#include "enemy.h"

typedef struct UIManager_S {
    GFC_List*       ui_list;
    int             active_button;
    GFC_Color       active_shift;
}UIManager;

static UIManager ui_manager = { 0 };

void ui_system_close();
Window* create_window(char* filename);
void create_button(Button* button, SJson* data);
void create_bar(Bar* bar, SJson* data);
void delete_window(Window* win);
void window_check_input(Window* win);

void ui_system_init(char* configFile) {
    SJson* config, *win_list;
    char* path;
    int i;
    
    // initialize all menus
    config = sj_load(configFile);
    if (!config) return;

    ui_manager.ui_list = gfc_list_new();

    win_list = sj_object_get_value(config, "win_list");
    for (i = 0; i < win_list->v.array->count; i++) {
        path = sj_object_get_string(sj_array_get_nth(win_list, i), "path");
        gfc_list_append(ui_manager.ui_list, create_window(path));
    }

    ui_manager.active_shift = sj_object_get_color(config, "active_shift");
    ui_manager.active_button = 0;
    sj_free(config);
    atexit(ui_system_close);
}

void ui_system_close() {
    gfc_list_foreach(ui_manager.ui_list, delete_window);
    gfc_list_delete(ui_manager.ui_list);

    memset(&ui_manager, 0, sizeof(UIManager));
}

Window* create_window(char* filename) {
    SJson* data, *win_data;
    Window* window;
    char buffer[100];
    int i;

    data = sj_load(filename);
    if (!data) {
        slog("failed to load window file");
        return NULL;
    }

    window = gfc_allocate_array(sizeof(Window), 1);
    if (!window) {
        slog("failed to allocate data for window");
        sj_free(data);
        return NULL;
    }

    win_data = sj_object_get_value(data, "window");
    sj_object_get_vector2d(win_data, "offset", &window->offset);

    strcpy(buffer, sj_object_get_string(win_data, "bg_sprite"));
    if (strcmp(buffer, "no sprite")) {
        window->bg_sprite = gf2d_sprite_load_image(buffer);
        window->dimensions = gfc_rect(window->offset.x, window->offset.y, window->bg_sprite->frame_w, window->bg_sprite->frame_h);
        //slog("window sprite initialized");
    }
    //else slog("no sprite provided");

    // create buttons
    sj_object_get_uint8(win_data, "buttonMax", &window->buttonMax);
    if (window->buttonMax) {
        window->buttonList = gfc_allocate_array(sizeof(Button), window->buttonMax);

        sj_object_get_int(win_data, "buttonLayout", &i);    
        window->button_layout = (ButtonLayout) i;

        win_data = sj_object_get_value(data, "buttons");
        for (i = 0; i < window->buttonMax; i++) {
            create_button(&window->buttonList[i], sj_array_get_nth(win_data, i));
        }
    }

    // create bars
    sj_object_get_uint8(win_data, "barMax", &window->barMax);
    if (window->barMax) {
        window->barList = gfc_allocate_array(sizeof(Bar), window->barMax);
        win_data = sj_object_get_value(data, "bars");
        for (i = 0; i < window->barMax; i++) {
            create_bar(&window->barList[i], sj_array_get_nth(win_data, i));
        }
    }

    sj_free(data);
    return window;
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

void create_bar(Bar* bar, SJson* data) {
    if (!bar || !data) {
        slog("couldn't create bar");
        return;
    }
    bar->sprite = gf2d_sprite_load_image(sj_object_get_string(data, "sprite"));
    sj_object_get_vector2d(data, "offset", &bar->offset);
    bar->scale = gfc_vector2d(1, 1);
}

void delete_window(Window* window) {
    int i;

    gf2d_sprite_delete(window->bg_sprite);
    if (window->buttonMax) {
        for (i = 0; i < window->buttonMax; i++) {
            gf2d_sprite_delete(window->buttonList[i].sprite);
        }
        free(window->buttonList);
    }
    if (window->barMax) {
        for (i = 0; i < window->barMax; i++) {
            gf2d_sprite_delete(window->barList[i].sprite);
        }
        free(window->barList);
    }
    free(window);
    slog("deleted window");
}

void drawUI(Uint8 w_state) {
    WorldState world_state;
    PlayerData* p_data;
    EnemyData* e_data;
    GFC_List* enemy_list;
    Entity* enemy;
    GFC_Color* shift;
    Window* window;
    Button* button;
    Bar* bar;
    int i, j;

    world_state = (WorldState) w_state;
    shift = NULL;

    window = (Window*)gfc_list_nth(ui_manager.ui_list, w_state);
    if (world_state != WORLD_INGAME) {
        gf2d_sprite_draw_image(window->bg_sprite, window->offset); // draw bg
        window_check_input(window);

        // draw buttons
        for (i = 0; i < window->buttonMax; i++) {
            button = &window->buttonList[i];
            if (i == ui_manager.active_button) shift = &ui_manager.active_shift;
            gf2d_sprite_draw(button->sprite, button->offset, NULL, NULL, NULL, NULL, shift, 0);
            shift = NULL;

            // check if button has been selected
            if (button->selected) {
                button->selected = 0;
                switch (world_state) {
                    case WORLD_MAINMENU:
                        if (!strcmp(button->cmd, "PLAY"))
                            change_world_state(WORLD_GAMESTART);
                        else if (!strcmp(button->cmd, "QUIT"))
                            change_world_state(WORLD_CLOSE);

                        break;

                    case WORLD_PAUSEMENU:
                        if (!strcmp(button->cmd, "RESUME"))
                            change_world_state(WORLD_INGAME);
                        else if (!strcmp(button->cmd, "QUIT"))
                            change_world_state(WORLD_GAMECLOSE);

                        break;

                    case WORLD_LEVELCOMPLETE:
                        if (!strcmp(button->cmd, "NEXT")) // load next level here
                            change_world_state(WORLD_LOAD_NEXT_LEVEL);
                        else if (!strcmp(button->cmd, "QUIT"))
                            change_world_state(WORLD_GAMECLOSE);

                        break;

                    case WORLD_PLAYERDEAD:
                        if (!strcmp(button->cmd, "RESPAWN")) // load next level here
                            change_world_state(WORLD_RESTART_LEVEL);
                        else if (!strcmp(button->cmd, "QUIT"))
                            change_world_state(WORLD_GAMECLOSE);

                        break;

                    case WORLD_GAME_COMPLETE:
                        if (!strcmp(button->cmd, "QUIT"))
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
        for (i = 0; i < window->barMax; i++) {
            bar = &window->barList[i];
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

void window_check_input(Window* window) {
    Button* active_button;

    if (!window) {
        slog("window is null");
        return;
    }

    // window traversal
    switch (window->button_layout) {
        case WINDOW_BUTTON_HORIZONTAL:
            if (gfc_input_command_pressed("moveright")) {
                if (ui_manager.active_button < window->buttonMax - 1)
                    ui_manager.active_button++;
            }
            else if (gfc_input_command_pressed("moveleft")) {
                if (ui_manager.active_button)
                    ui_manager.active_button--;
            }

            break;

        case WINDOW_BUTTON_VERTICAL:
            if (gfc_input_command_pressed("moveup")) {
                if (ui_manager.active_button) 
                    ui_manager.active_button--;
            }
            else if (gfc_input_command_pressed("movedown")) {
                if (ui_manager.active_button < window->buttonMax - 1)
                    ui_manager.active_button++;
            }

            break;

        case WINDOW_BUTTON_CARDINAL: //TODO: fix later
            if (gfc_input_command_pressed("moveup")) {
                if (ui_manager.active_button)
                    ui_manager.active_button--;
            }
            else if (gfc_input_command_pressed("movedown")) {
                if (ui_manager.active_button < window->buttonMax - 1)
                    ui_manager.active_button++;
            }
            else if (gfc_input_command_pressed("moveright")) {
                if (ui_manager.active_button < window->buttonMax - 1)
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
        active_button = &window->buttonList[ui_manager.active_button];
        active_button->selected = 1;
    }
}
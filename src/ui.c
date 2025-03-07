#include "ui.h"

typedef struct UIManager_S {
    GFC_List*       menu_list;
}UIManager;

static UIManager ui_manager = { 0 };

void ui_system_close();

void ui_system_init() {
    

    atexit(ui_system_close);
}

void ui_system_close() {


    memset(&ui_manager, 0, sizeof(UIManager));
}
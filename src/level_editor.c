#include "simple_logger.h"
#include "gfc_config.h"
#include "world.h"
#include "collisions.h"
#include "player.h"
#include "enemy.h"
#include "item.h"

typedef struct EditorManager_S {
    float test;
}EditorManager;

void level_editor_init(){
    SJson* config;

    config = sj_load("level_editor.cfg");
    if (!config) {
        slog("cannot load level_editor.cfg");
        change_world_state(WORLD_MAINMENU);
        return;
    }

    

    sj_free(config);
}

void level_editor_update() {

}

void level_editor_close(){
    
}


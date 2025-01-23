#include "simple_logger.h"
#include "gfc_matrix.h"
#include "entity.h"

typedef struct EntityManager_S {
	Entity*		entityList;
	Uint32		entityMax;
}EntityManager;

static EntityManager ent_manager = { 0 };

void entity_system_init(Uint32 maxEnts) {
    //sanity check
    if (ent_manager.entityList) {
        slog("entity manager already exists");
        return;
    }

    //another sanity check
    if (!maxEnts) {
        slog("cannot allocated 0 entities for the entity manager");
        return;
    }
    ent_manager.entityList = gfc_allocate_array(sizeof(Entity), maxEnts);
    if (!ent_manager.entityList) {
        slog("failed to allocate %i entities for the entity manager", maxEnts);
        return;
    }
    ent_manager.entityMax = maxEnts; // at this point, big ass entity list is made

    atexit(entity_system_close);
}

void entity_system_close() {

}
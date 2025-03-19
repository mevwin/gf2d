#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "gfc_shape.h"

void camera_init();
void camera_update(void* p, GFC_Vector2D level_size);

#endif
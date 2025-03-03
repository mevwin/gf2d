#ifndef __UI_H__
#define __UI_H__

#include "gf2d_sprite.h"
#include "gfc_shape.h"

typedef struct Button_S {
    Sprite*         sprite;
    GFC_Rect        dimensions;
    //GFC_TextLine    text;
}Button;

typedef struct Menu_S {
    Sprite*         background;
    GFC_Rect        dimensions;
    Button*         buttonList;
    Uint8           buttonMax;
    //GFC_TextLine    text;
}Menu;



void ui_system_init();

#endif
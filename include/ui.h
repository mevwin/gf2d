#ifndef __UI_H__
#define __UI_H__

#include "gf2d_sprite.h"
#include "gfc_shape.h"
#include "font.h"

typedef enum ButtonLayout_E {
    MENU_BUTTON_HORIZONTAL,
    MENU_BUTTON_VERTICAL,
    MENU_BUTTON_CARDINAL
}ButtonLayout;

typedef struct TextLine_S {
    GFC_TextLine    text;
    FontType        type;
    FontSize        size;
    GFC_Color       color;
}TextLine;

typedef struct Button_S {
    TextLine        textline;
    GFC_Vector2D    offset;
    GFC_Rect        region;
    GFC_Color       color;
    Uint8           selected;
}Button;

typedef struct Bar_S {
    Sprite*         sprite;
    //TextBlock       textblock;
    GFC_Vector2D    offset;
    GFC_Vector2D    scale;
}Bar;

typedef struct Menu_S {
    Sprite*         bg_sprite;
    GFC_Vector2D    offset;
    GFC_Rect        dimensions;

    ButtonLayout    button_layout;
    Uint8           buttonMax;
    Button*         buttonList;   

    Uint8           barMax;
    Bar*            barList;

    //Uint8           textblockMax;
    //TextBlock*      textblockList;
}Menu;

void ui_system_init(char* configFile);
void drawUI(Uint8 w_state);

#endif
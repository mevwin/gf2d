#ifndef __UI_H__
#define __UI_H__

#include "gf2d_sprite.h"
#include "gfc_shape.h"

typedef enum ButtonLayout_E {
    WINDOW_BUTTON_HORIZONTAL,
    WINDOW_BUTTON_VERTICAL,
    WINDOW_BUTTON_CARDINAL
}ButtonLayout;

typedef struct Button_S {
    char            cmd[20];
    Sprite*         sprite;
    GFC_Vector2D    offset;
    GFC_Rect        region;
    Uint8           selected;

    //GFC_TextLine    text;
}Button;

typedef struct Bar_S {
    
    Sprite*         sprite;
    GFC_Vector2D    offset;
    GFC_Vector2D    scale;
}Bar;

typedef struct Window_S {
    Sprite*         bg_sprite;
    GFC_Vector2D    offset;
    GFC_Rect        dimensions;
    ButtonLayout    button_layout;
    Uint8           buttonMax;
    Button*         buttonList;   
    Uint8           barMax;
    Bar*            barList;
    //GFC_TextLine    text;
}Window;

void ui_system_init(char* configFile);
void drawUI(Uint8 w_state);

#endif
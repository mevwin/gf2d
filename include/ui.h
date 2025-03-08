#ifndef __UI_H__
#define __UI_H__

#include "gf2d_sprite.h"
#include "gfc_shape.h"

typedef enum ButtonLayout_E {
    MENU_BUTTON_HORIZONTAL,
    MENU_BUTTON_VERTICAL,
    MENU_BUTTON_CARDINAL
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
}Bar;

typedef struct Menu_S {
    Sprite*         bg_sprite;
    GFC_Vector2D    offset;
    GFC_Rect        dimensions;
    ButtonLayout    button_layout;
    Uint8           buttonMax;
    Button*         buttonList;    
    //GFC_TextLine    text;
}Menu;

typedef struct PlayerHUD_S {
    Bar*            barList;
}PlayerHUD;

void ui_system_init(char* configFile);
void drawUI(Uint8 w_state);



#endif
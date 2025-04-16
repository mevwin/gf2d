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

typedef enum ButtonType_E {
    BUTTON_MENU,
    BUTTON_SCROLL
}ButtonType;

typedef enum WindowType_E {
    WINDOW_NOTIF,               // exists for some ttl
    WINDOW_BLOCK                // exists forever
}WindowType;

typedef struct TextLine_S {
    GFC_TextLine    text;
    FontType        type;
    FontSize        size;
    GFC_Color       color;
    GFC_Vector2D    offset;
}TextLine;

typedef struct Button_S {
    ButtonType      type;
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

typedef struct WindowButtonData {
    Uint8           drawn;
    GFC_TextWord    button_name;
    GFC_Vector2D    button_offset;
    void            (*effect)();
}WindowButtonData;

typedef struct Window_S {
    GFC_TextWord        name;
    WindowType          type;

    Uint8               toggled;
    float               ttl_then;
    int                 ttl_counter;
    int                 ttl;                // if ttl = -1, live forever
    TextLine            textline;

    GFC_Vector2D        offset;
    GFC_Vector2D        scale;
    GFC_Color           color;

    Uint8               button_count;
    WindowButtonData*   wbd;
}Window;

typedef struct Menu_S {
    GFC_TextWord    name;
    Sprite*         bg_sprite;

    ButtonLayout    button_layout;
    Uint8           buttonMax;
    Button*         buttonList;   

    Uint8           barMax;
    Bar*            barList;

    Uint8           windowMax;
    Window*         windowList;
}Menu;

/**
* NOTE: difference between menu and window
* menu covers the whole screen
* window covers only a small portion
* a window can exist on its on without a menu
*/

void ui_system_init(char* configFile);
void drawUI(Uint8 w_state);
void toggle_window(const char* name, Uint8 toggle);

#endif
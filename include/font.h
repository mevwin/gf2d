#ifndef __FONT_H__
#define __FONT_H__

#include <string.h>
#include <SDL_ttf.h>

#include "gfc_shape.h"
#include "gfc_vector.h"

typedef enum FontType_E {
	FONT_HEADING,
	FONT_STANDARD
}FontType;

typedef enum FontSize_E {
	FONT_SIZE_SMALL,
	FONT_SIZE_MEDIUM,
	FONT_SIZE_LARGE,
	FONT_SIZE_HEADING
}FontSize;

typedef struct Font_S {
	TTF_Font*	font;
	FontType	type;
	Uint32		size;
}Font;

void font_system_init();
void font_display_text(
	const char* text,
	FontType type,
	FontSize size,
	GFC_Color color,
	Uint8 center,
	GFC_Vector2D* offset,
	GFC_Rect* rect_to_cent);
int get_font_size(Uint8 tag);

#endif
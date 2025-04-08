#ifndef __FONT_H__
#define __FONT_H__

#include <string.h>
#include <SDL_ttf.h>

#include "gfc_vector.h"

typedef enum FontType_E {
	FONT_HEADING,
	FONT_STANDARD
}FontType;

typedef enum FontSize_E {
	FONT_STYLE_SMALL = 10,
	FONT_STYLE_MEDIUM = 14,
	FONT_STYLE_LARGE = 28,
	FONT_STYLE_HEADING = 34
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
	GFC_Vector2D offset,
	GFC_Color color);

#endif
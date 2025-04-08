#include "simple_logger.h"
#include "gfc_config.h"
#include "gf2d_graphics.h"
#include "font.h"

typedef struct FontManager_S {
	Uint32			fontMax;
	Font*			fontList;
}FontManager;

static FontManager font_manager = { 0 };

void font_system_close();
Font* get_font(FontType type, FontSize size);

void font_system_init() {
	SJson* config, *font_list, *entry;
	int i;

	if (TTF_Init() == -1) {
		slog("failed to initialize TTF");
		return;
	}

	config = sj_load("config/font_system.cfg");
	if (!config) {
		slog("could not locate font_system.cfg");
		TTF_Quit();
		return;
	}

	font_list = sj_object_get_value(config, "font_list");
	font_manager.fontMax = font_list->v.array->count;
	font_manager.fontList = gfc_allocate_array(sizeof(Font), font_manager.fontMax);

	for (i = 0; i < font_manager.fontMax; i++) {
		entry = sj_array_nth(font_list, i);
		if (!entry) continue;

		sj_object_get_int(entry, "size", &font_manager.fontList[i].size);
		font_manager.fontList[i].font = TTF_OpenFont(sj_object_get_string(entry, "font"), font_manager.fontList[i].size);
		if (!font_manager.fontList[i].font) slog("failed to initialize font: %s", sj_object_get_string(entry, "font"));

		if (font_manager.fontList[i].size == FONT_STYLE_HEADING)
			font_manager.fontList[i].type = FONT_HEADING;
		else
			font_manager.fontList[i].type = FONT_STANDARD;
	}

	slog("font system initialized");
	sj_free(config);
	atexit(font_system_close);
}

Font* get_font(FontType type, FontSize size) {
	int i;
	Font* font;

	for (i = 0; i < font_manager.fontMax; i++) {
		font = &font_manager.fontList[i];
		if (!font->font) continue;

		if (font->type == type && font->size == size)
			return font;
	}
	return NULL;
}

void font_display_text(
	const char* text, 
	FontType type, 
	FontSize size, 
	GFC_Vector2D offset,
	GFC_Color color) 
{
	Font* font;
	SDL_Surface* surface;
	SDL_Color fg;
	SDL_Texture* texture;
	SDL_Rect rect;

	if (!TTF_WasInit())
		return;

	font = &font_manager.fontList[3];
	//get_font(type, size);
	if (!font) {
		slog("couldn't find font of type: %d and size %d", type, size);
		return;
	}

	fg = gfc_color_to_sdl(color);
	surface = TTF_RenderUTF8_Blended_Wrapped(font->font, text, fg, 0);
	if (!surface) {
		slog("failed to initialize surface from parameters");
		return;
	}

	surface = gf2d_graphics_screen_convert(&surface);
	texture = SDL_CreateTextureFromSurface(gf2d_graphics_get_renderer(), surface);
	if (!texture) {
		SDL_FreeSurface(surface);
		slog("failed to create texture from surface");
		return;
	}

	rect.x = offset.x;
	rect.y = offset.y;
	rect.w = surface->w;
	rect.h = surface->h;

	SDL_RenderCopy(gf2d_graphics_get_renderer(), texture, NULL, &rect);

	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}

void font_system_close() {
	int i;

	if (!font_manager.fontList) return;

	for (i = 0; i < font_manager.fontMax; i++) {
		if (!font_manager.fontList[i].font) continue;

		TTF_CloseFont(font_manager.fontList[i].font);
	}
	free(font_manager.fontList);

	memset(&font_manager, 0, sizeof(FontManager));
	TTF_Quit();
	slog("font system closed");
}
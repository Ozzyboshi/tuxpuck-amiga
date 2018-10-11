/* font.c - Copyright (C) 2001-2002 Jacob Kroon, see COPYING for details */

#include <stdlib.h>
#include <string.h>
#include "video.h"
#include "font.h"

/* structs */
typedef struct _Glyph Glyph;

struct _Glyph {
  Sint16 x_move, y_move, advance;
  SDL_Surface *sdl_image;
};

struct _Font {
  Glyph **glyph;
  Uint16 x, y;
};

/* statics */
static Uint8 _alpha = 255;

/* functions */
Font *font_create(Uint8 * data, Uint32 * memcounter)
{
  Uint32 index = 0, i, size;
  Uint8 ch;
  Sint16 x_size, y_size;
  SDL_Color palette[256];
  Font *font = NULL;
    printf("%d\n",(int)data[0]);
  printf("%d\n",(int)data[1]);
  printf("%d\n",(int)data[2]);
  printf("%d\n",(int)data[3]);

  revmemcpy(&size, data, sizeof(Uint32));
  if (memcounter)
    *memcounter += size + sizeof(Uint32);
  data += sizeof(Uint32);
  font = (Font *) malloc(sizeof(Font));
  for (i = 0; i < 256; i++)
    palette[i].r = palette[i].g = palette[i].b = i;
  font->glyph = (Glyph **) malloc(256 * sizeof(Glyph *));
  memset(font->glyph, 0, 256 * sizeof(Glyph *));
#ifdef _DEBUG
    printf("Font create size is (%d)!!!!!!!uint32 is %d size of unsigned char is %d\n",size,sizeof(Uint32),sizeof(unsigned char));
#endif
  while (index < size) {
    ch = data[index++];
    revmemcpy(&x_size, &data[index], 2);
    index += 2;
    revmemcpy(&y_size, &data[index], 2);
    index += 2;
    font->glyph[ch] = (Glyph *) malloc(sizeof(Glyph));
    revmemcpy(&font->glyph[ch]->x_move, &data[index], 2);
    index += 2;
    revmemcpy(&font->glyph[ch]->y_move, &data[index], 2);
    index += 2;
    revmemcpy(&font->glyph[ch]->advance, &data[index], 2);
    index += 2;
    font->glyph[ch]->sdl_image =
      SDL_CreateRGBSurfaceFrom(&data[index], x_size, y_size, 8, x_size, 0,
			       0, 0, 0);
    if ( font->glyph[ch]->sdl_image == NULL)
    {
	printf("Failed to load data at index %d\n",index);
	exit(1);
    }
    SDL_SetPalette(font->glyph[ch]->sdl_image, SDL_LOGPAL, palette, 0, 256);
    SDL_SetColorKey(font->glyph[ch]->sdl_image, SDL_SRCCOLORKEY, 0);
    index += x_size * y_size;
  }
  font->x = font->y = 0;
  return font;
}

void font_free(Font * font)
{
  Uint32 i;

  for (i = 0; i < 256; i++)
    if (font->glyph[i]) {
      SDL_FreeSurface(font->glyph[i]->sdl_image);
      free(font->glyph[i]);
    }
  free(font->glyph);
  free(font);
}

void font_set_color(Font * font, Uint8 r, Uint8 g, Uint8 b)
{
  Uint32 i;
  SDL_Color palette[256];

  for (i = 0; i < 256; i++) {
    palette[i].r = (Uint8) (r * i / 256.0);
    palette[i].g = (Uint8) (g * i / 256.0);
    palette[i].b = (Uint8) (b * i / 256.0);
  }
  for (i = 0; i < 256; i++)
    if (font->glyph[i])
      SDL_SetPalette(font->glyph[i]->sdl_image, SDL_LOGPAL, palette, 0, 256);
}

void font_set_alpha(Font * font, Uint8 alpha)
{
  _alpha = alpha;
}

void font_print(Font * font, char *string, SDL_Rect * fill)
{
  Uint32 ruler = 0, i;
  SDL_Rect rect, area;
#ifdef _DEBUG
			printf("Printing text:'%s'\n",string);
#endif
  for (i = 0; i < strlen(string); i++)
    if (font->glyph[(Uint8) string[i]]) {
      rect.x = font->x + font->glyph[(Uint8) string[i]]->x_move + ruler;
      rect.y = font->y - font->glyph[(Uint8) string[i]]->y_move;
      rect.w = rect.h = 0;
      video_set_alpha(font->glyph[(Uint8) string[i]]->sdl_image, _alpha);
      video_blit(font->glyph[(Uint8) string[i]]->sdl_image, NULL, &rect);
      if (i == 0)
	area = rect;
      else {
	if (rect.x < area.x)
	  area.x = rect.x;
	if (rect.y < area.y)
	  area.y = rect.y;
	if (rect.x + rect.w > area.x + area.w)
	  area.w = rect.x + rect.w - area.x;
	if (rect.y + rect.h > area.y + area.h)
	  area.h = rect.y + rect.h - area.y + 1;
      }
      ruler += font->glyph[(Uint8) string[i]]->advance-3;
    }
#ifdef _DEBUG
    else printf("Glyph NOT FOUND for charater %c(%d)!!!!!!!\n",string[i],(Uint8) string[i]);
#endif
  if (fill)
    *fill = area;
}

Uint32 font_calc_width(Font * font, char *string)
{
  Uint32 width = 0, i;

  if (string == NULL)
    return 0;
  for (i = 0; i < strlen(string); i++)
    width += font->glyph[(Uint8) string[i]]->advance-3;
  return width;
}

Uint32 font_calc_height(Font * font, char *string)
{
  Uint32 height = 0, i;
  Glyph *glyph;

  if (string == NULL)
    return 0;
  for (i = 0; i < strlen(string); i++) {
    glyph = font->glyph[(Uint8) string[i]];
    if (glyph->sdl_image->h > height)
      height = glyph->sdl_image->h;
  }
  return height-4;
}

void font_move_pen(Font * font, Uint16 dx, Uint16 dy)
{
  font->x += dx;
  font->y += dy;
}

void font_set_pen(Font * font, Uint16 x, Uint16 y)
{
  font->x = x;
  font->y = y;
}
void *
revmemcpy (void *dest, const void *src, size_t len)
{
  char *d = dest + len - 1;
  const char *s = src;
  while (len--)
    *d-- = *s++;
  return dest;
}

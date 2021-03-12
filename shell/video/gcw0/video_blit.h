#ifndef VIDEO_BLIT_H
#define VIDEO_BLIT_H

#include <SDL/SDL.h>

#define HOST_WIDTH_RESOLUTION sdl_screen->w
#define HOST_HEIGHT_RESOLUTION sdl_screen->h

#define BACKBUFFER_WIDTH_RESOLUTION backbuffer->w
#define BACKBUFFER_HEIGHT_RESOLUTION backbuffer->h

extern SDL_Surface *sdl_screen, *backbuffer;

extern uint32_t width_of_surface;
extern uint32_t* Draw_to_Virtual_Screen;

void Init_Video();
void Set_Video_Menu();
void Set_Video_InGame();
void Video_Close();
void Update_Video_Menu();
void Update_Video_Ingame();

#endif

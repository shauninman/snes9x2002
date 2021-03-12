/* Cygne
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Dox dox@space.pl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL/SDL.h>
#include <sys/time.h>
#include <sys/types.h>
#include "main.h"
#include "snes9x.h"
#include "soundux.h"
#include "memmap.h"
#include "apu.h"
#include "cheats.h"
#include "display.h"
#include "gfx.h"
#include "cpuexec.h"
#include "spc7110.h"
#include "srtc.h"
#include "sa1.h"
#include "scaler.h"

#include "video_blit.h"
#include "scaler.h"
#include "config.h"


SDL_Surface *sdl_screen, *backbuffer;

uint32_t width_of_surface;
uint32_t* Draw_to_Virtual_Screen;

#ifndef SDL_TRIPLEBUF
#define SDL_TRIPLEBUF SDL_DOUBLEBUF
#endif

#define SDL_FLAGS SDL_HWSURFACE | SDL_TRIPLEBUF

void Init_Video()
{
	SDL_Init( SDL_INIT_VIDEO );

	SDL_ShowCursor(0);

	sdl_screen = SDL_SetVideoMode(640, 480, 16, SDL_FLAGS);

	backbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0,0,0,0);

	Set_Video_InGame();
}

void Set_Video_Menu()
{
	sdl_screen = SDL_SetVideoMode(320, 240, 16, SDL_FLAGS);
}

void Set_Video_InGame()
{
	sdl_screen = SDL_SetVideoMode(IPPU.RenderedScreenWidth, IPPU.RenderedScreenHeight, 16, SDL_FLAGS);
}

void Video_Close()
{
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
	if (backbuffer) SDL_FreeSurface(backbuffer);
	SDL_Quit();
}

void Update_Video_Menu()
{
	SDL_BlitSurface(backbuffer, NULL, sdl_screen, NULL);
	SDL_Flip(sdl_screen);
}

void Update_Video_Ingame()
{
	uint32_t *s, *d;
	uint32_t h, w;
	uint8_t PAL = !!(Memory.FillRAM[0x2133] & 4);

	if (SDL_LockSurface(sdl_screen) == 0)
	{
		if (IPPU.RenderedScreenWidth != sdl_screen->w || IPPU.RenderedScreenHeight != sdl_screen->h) Set_Video_InGame();
		memcpy(sdl_screen->pixels, GFX.Screen, (IPPU.RenderedScreenWidth * IPPU.RenderedScreenHeight) * 2);
		SDL_UnlockSurface(sdl_screen);
	}
	SDL_Flip(sdl_screen);
}

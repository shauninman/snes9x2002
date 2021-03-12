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

void Init_Video()
{
	SDL_Init( SDL_INIT_VIDEO );
	
	SDL_ShowCursor(0);
	
	sdl_screen = SDL_SetVideoMode(0, 0, 16, SDL_HWSURFACE);
	
	backbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0,0,0,0);

	Set_Video_InGame();
}

void Set_Video_Menu()
{
}

void Set_Video_InGame()
{
}

void Video_Close()
{
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
	if (backbuffer) SDL_FreeSurface(backbuffer);
	SDL_Quit();
}

void Update_Video_Menu()
{
	SDL_SoftStretch(backbuffer, NULL, sdl_screen, NULL);
	SDL_Flip(sdl_screen);
}

void Update_Video_Ingame()
{
	uint32_t *s, *d;
	uint32_t h, w;
	uint8_t PAL = !!(Memory.FillRAM[0x2133] & 4);
	
	SDL_LockSurface(sdl_screen);
	
	switch(option.fullscreen)
	{
		case 0:
			bitmap_scale(0,0,IPPU.RenderedScreenWidth,IPPU.RenderedScreenHeight,IPPU.RenderedScreenWidth*2,sdl_screen->h, SNES_WIDTH, sdl_screen->w - (IPPU.RenderedScreenWidth*2),(uint16_t* restrict)GFX.Screen,(uint16_t* restrict)sdl_screen->pixels+(HOST_WIDTH_RESOLUTION-(IPPU.RenderedScreenWidth*2))/2+(HOST_HEIGHT_RESOLUTION-((HOST_HEIGHT_RESOLUTION)))/2*HOST_WIDTH_RESOLUTION);
		break;
		case 1:
		case 2:
			bitmap_scale(0, 0, IPPU.RenderedScreenWidth, IPPU.RenderedScreenHeight, sdl_screen->w, sdl_screen->h, SNES_WIDTH, 0, GFX.Screen, sdl_screen->pixels);
		break;
	}

	
	SDL_UnlockSurface(sdl_screen);	
	SDL_Flip(sdl_screen);
}

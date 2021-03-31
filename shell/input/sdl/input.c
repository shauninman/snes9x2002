#include <SDL/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "snes9x.h"
#include "soundux.h"
#include "memmap.h"
#include "apu.h"
#include "cheats.h"
#include "display.h"
#include "gfx.h"
#include "cpuexec.h"
#include "srtc.h"
#include "sa1.h"
#include "scaler.h"

#include "menu.h"
#include "config.h"
#include "buttons.h"

uint8_t *keystate;
uint8_t exit_snes = 0;
extern uint32_t emulator_state;

static uint32_t joypad = 0;

uint32_t S9xReadJoypad(int32_t port)
{
	// Only 1P is supported
	if (port != 0) return 0;

	SDL_Event event;
	static const uint32_t snes_lut[] =
	{
		SNES_B_MASK,
		SNES_Y_MASK,
		SNES_SELECT_MASK,
		SNES_START_MASK,
		SNES_UP_MASK,
		SNES_DOWN_MASK,
		SNES_LEFT_MASK,
		SNES_RIGHT_MASK,
		SNES_A_MASK,
		SNES_X_MASK,
		SNES_TL_MASK,
		SNES_TR_MASK
	};

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_KEYDOWN: {
				SDLKey key = event.key.keysym.sym;
				
				if (key==BTN_MENU) 
					emulator_state = 1;
				
				if (key==option.config_buttons[0][0])
					joypad |= SNES_UP_MASK;
				if (key==option.config_buttons[0][1])
					joypad |= SNES_RIGHT_MASK;
				if (key==option.config_buttons[0][2])
					joypad |= SNES_DOWN_MASK;
				if (key==option.config_buttons[0][3])
					joypad |= SNES_LEFT_MASK;
				
				if (key==option.config_buttons[0][4])
					joypad |= SNES_A_MASK;
				if (key==option.config_buttons[0][5])
					joypad |= SNES_B_MASK;
				if (key==option.config_buttons[0][6])
					joypad |= SNES_X_MASK;
				if (key==option.config_buttons[0][7])
					joypad |= SNES_Y_MASK;
				if (key==option.config_buttons[0][8])
					joypad |= SNES_TL_MASK;
				if (key==option.config_buttons[0][9])
					joypad |= SNES_TR_MASK;
				
				if (key==option.config_buttons[0][10])
					joypad |= SNES_START_MASK;
				if (key==option.config_buttons[0][11])
					joypad |= SNES_SELECT_MASK;
			} break;
				
			
			case SDL_KEYUP: {
				SDLKey key = event.key.keysym.sym;
				
				if (key==option.config_buttons[0][0])
					joypad &= ~SNES_UP_MASK;
				if (key==option.config_buttons[0][1])
					joypad &= ~SNES_RIGHT_MASK;
				if (key==option.config_buttons[0][2])
					joypad &= ~SNES_DOWN_MASK;
				if (key==option.config_buttons[0][3])
					joypad &= ~SNES_LEFT_MASK;
				
				if (key==option.config_buttons[0][4])
					joypad &= ~SNES_A_MASK;
				if (key==option.config_buttons[0][5])
					joypad &= ~SNES_B_MASK;
				if (key==option.config_buttons[0][6])
					joypad &= ~SNES_X_MASK;
				if (key==option.config_buttons[0][7])
					joypad &= ~SNES_Y_MASK;
				if (key==option.config_buttons[0][8])
					joypad &= ~SNES_TL_MASK;
				if (key==option.config_buttons[0][9])
					joypad &= ~SNES_TR_MASK;
				
				if (key==option.config_buttons[0][10])
					joypad &= ~SNES_START_MASK;
				if (key==option.config_buttons[0][11])
					joypad &= ~SNES_SELECT_MASK;
			} break;
		}
	}

	return joypad;
}

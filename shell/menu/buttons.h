#ifndef BUTTONS_H
#define BUTTONS_H

#ifdef TRIMUI
#include "buttons_trimui.h"
#else

#include <SDL/SDL.h>

#define BTN_UP         SDLK_UP
#define BTN_RIGHT      SDLK_RIGHT
#define BTN_DOWN       SDLK_DOWN
#define BTN_LEFT       SDLK_LEFT
#define BTN_A          SDLK_LCTRL
#define BTN_B          SDLK_LALT
#define BTN_X          SDLK_LSHIFT
#define BTN_Y          SDLK_SPACE
#define BTN_L          SDLK_TAB
#define BTN_R          SDLK_BACKSPACE
#define BTN_START      SDLK_RETURN
#define BTN_SELECT     SDLK_ESCAPE
#define BTN_MENU       SDLK_RCTRL

#define BTN_L2         SDLK_END
#define BTN_R2         SDLK_3
#define BTN_VOLUMEUP   SDLK_AMPERSAND
#define BTN_VOLUMEDOWN SDLK_WORLD_73
#define BTN_HOME       SDLK_HOME
#endif

#endif // BUTTONS_H

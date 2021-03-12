#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <SDL/SDL.h>

#include "scaler.h"
#include "font_drawing.h"
#include "sound_output.h"
#include "video_blit.h"
#include "config.h"
#include "menu.h"
#include "buttons.h"

t_config option;
uint32_t emulator_state = 0;

extern uint8_t exit_snes;

static char home_path[256], save_path[256], sram_path[256], conf_path[256], rtc_path[256];
static uint32_t controls_chosen = 0;

extern SDL_Surface *sdl_screen;
extern char GameName_emu[512];

extern void SRAM_file(char* path, uint_fast8_t state);
extern void SaveState(char* path, uint_fast8_t state);
extern void SRAM_Save(char* path, uint_fast8_t state);

static uint8_t selectpressed = 0;
static uint8_t save_slot = 0;
static const int8_t upscalers_available = 2
#ifdef SCALE2X_UPSCALER
+1
#endif
;

static void SaveState_Menu(uint_fast8_t load_mode, uint_fast8_t slot)
{
	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%s/%s_%d.sts", save_path, GameName_emu, slot);
	SaveState(tmp,load_mode);
}

static void SRAM_Menu(uint_fast8_t load_mode)
{
	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%s/%s.srm", sram_path, GameName_emu);
	SRAM_Save(tmp,load_mode);
}

static void config_load()
{
	uint_fast8_t i;
	char config_path[512];
	FILE* fp;
	snprintf(config_path, sizeof(config_path), "%s/%s.cfg", conf_path, GameName_emu);

	fp = fopen(config_path, "rb");
	if (fp)
	{
		fread(&option, sizeof(option), sizeof(int8_t), fp);
		fclose(fp);
	}
	else
	{
		/* Default mapping for Horizontal */
		option.config_buttons[0][0] = BTN_UP;
		option.config_buttons[0][1] = BTN_RIGHT;
		option.config_buttons[0][2] = BTN_DOWN;
		option.config_buttons[0][3] = BTN_LEFT;

		option.config_buttons[0][4] = BTN_A;
		option.config_buttons[0][5] = BTN_B;
		option.config_buttons[0][6] = BTN_X;
		option.config_buttons[0][7] = BTN_Y;

		option.config_buttons[0][8] = BTN_L;
		option.config_buttons[0][9] = BTN_R;

		option.config_buttons[0][10] = BTN_START;
		option.config_buttons[0][11] = BTN_SELECT;

		option.fullscreen = 0;
	}
}

static void config_save()
{
	FILE* fp;
	char config_path[512];
	snprintf(config_path, sizeof(config_path), "%s/%s.cfg", conf_path, GameName_emu);

	fp = fopen(config_path, "wb");
	if (fp)
	{
		fwrite(&option, sizeof(option), sizeof(int8_t), fp);
		fclose(fp);
	}
}

static const char* Return_Text_Button(uint32_t button)
{
	switch(button)
	{
		/* UP button */
		case BTN_UP:
			return "DPAD UP";
		break;
		/* DOWN button */
		case BTN_DOWN:
			return "DPAD DOWN";
		break;
		/* LEFT button */
		case BTN_LEFT:
			return "DPAD LEFT";
		break;
		/* RIGHT button */
		case BTN_RIGHT:
			return "DPAD RIGHT";
		break;
		/* A button */
		case BTN_A:
			return "A";
		break;
		/* B button */
		case BTN_B:
			return "B";
		break;
		/* X button */
		case BTN_X:
			return "X";
		break;
		/* Y button */
		case BTN_Y:
			return "Y";
		break;
		/* L button */
		case BTN_L:
			return "L";
		break;
		/* R button */
		case BTN_R:
			return "R";
		break;
		/* Power button */
		case BTN_L2:
			return "L2";
		break;
		/* Brightness */
		case BTN_R2:
			return "R2";
		break;
		/* Volume - */
		case BTN_VOLUMEUP:
			return "Volume -";
		break;
		/* Volume + */
		case BTN_VOLUMEDOWN:
			return "Volume +";
		break;
		/* Start */
		case BTN_START:
			return "Start";
		break;
		/* Select */
		case BTN_SELECT:
			return "Select";
		break;
		default:
			return "Unknown";
		break;
		case 0:
			return "...";
		break;
	}
}

static void Input_Remapping()
{
	SDL_Event Event;
	char text[50];
	uint32_t pressed = 0;
	int32_t currentselection = 1;
	int32_t exit_input = 0;
	uint32_t exit_map = 0;

	while(!exit_input)
	{
		pressed = 0;
		SDL_FillRect( backbuffer, NULL, 0 );

        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_KEYDOWN)
            {
                switch(Event.key.keysym.sym)
                {
                    case BTN_UP:
                        currentselection--;
                        if (currentselection < 1)
                        {
							if (currentselection > 9) currentselection = 12;
							else currentselection = 9;
						}
                        break;
                    case BTN_DOWN:
                        currentselection++;
                        if (currentselection == 10)
                        {
							currentselection = 1;
						}
                        break;
                    case BTN_A:
                    case BTN_START:
                        pressed = 1;
					break;
                    case BTN_MENU:
                        option.config_buttons[controls_chosen][currentselection - 1] = 0;
					break;
                    case BTN_B:
                        exit_input = 1;
					break;
                    case BTN_LEFT:
						if (currentselection > 9) currentselection -= 9;
					break;
                    case BTN_RIGHT:
						if (currentselection < 10) currentselection += 9;
					break;
                    case BTN_R:
						controls_chosen = 1;
					break;
                    case BTN_L:
						controls_chosen = 0;
					break;
					default:
					break;
                }
            }
        }

        if (pressed)
        {
			SDL_Delay(1);
            switch(currentselection)
            {
                default:
					exit_map = 0;
					while( !exit_map )
					{
						SDL_FillRect( backbuffer, NULL, 0 );
						print_string("Please press button for mapping", TextWhite, TextBlue, 37, 108, backbuffer->pixels);
						while (SDL_PollEvent(&Event))
						{
							if (Event.type == SDL_KEYDOWN)
							{
								if (Event.key.keysym.sym != BTN_MENU)
								{
									option.config_buttons[controls_chosen][currentselection - 1] = Event.key.keysym.sym;
									exit_map = 1;
								}
							}
						}
						Update_Video_Menu();
					}
				break;
            }
        }

        if (currentselection > 12) currentselection = 12;

		if (controls_chosen == 0) print_string("Player 1", TextWhite, 0, 100, 10, backbuffer->pixels);
		else print_string("Player 2", TextWhite, 0, 100, 10, backbuffer->pixels);

		print_string("Press [A] to map to a button", TextWhite, TextBlue, 50, 210, backbuffer->pixels);
		print_string("Press [B] to Exit", TextWhite, TextBlue, 85, 225, backbuffer->pixels);

		snprintf(text, sizeof(text), "UP   : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][0]));
		if (currentselection == 1) print_string(text, TextRed, 0, 5, 25+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 25+2, backbuffer->pixels);

		snprintf(text, sizeof(text), "DOWN : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][1]));
		if (currentselection == 2) print_string(text, TextRed, 0, 5, 45+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 45+2, backbuffer->pixels);

		snprintf(text, sizeof(text), "LEFT : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][2]));
		if (currentselection == 3) print_string(text, TextRed, 0, 5, 65+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 65+2, backbuffer->pixels);

		snprintf(text, sizeof(text), "RIGHT: %s\n", Return_Text_Button(option.config_buttons[controls_chosen][3]));
		if (currentselection == 4) print_string(text, TextRed, 0, 5, 85+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 85+2, backbuffer->pixels);

		snprintf(text, sizeof(text), "A    : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][4]));
		if (currentselection == 5) print_string(text, TextRed, 0, 5, 105+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 105+2, backbuffer->pixels);

		snprintf(text, sizeof(text), "B    : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][5]));
		if (currentselection == 6) print_string(text, TextRed, 0, 5, 125+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 125+2, backbuffer->pixels);

		snprintf(text, sizeof(text), "X    : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][6]));
		if (currentselection == 7) print_string(text, TextRed, 0, 5, 145+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 145+2, backbuffer->pixels);

		snprintf(text, sizeof(text), "Y    : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][7]));
		if (currentselection == 8) print_string(text, TextRed, 0, 5, 165+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 165+2, backbuffer->pixels);

		snprintf(text, sizeof(text), "L    : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][8]));
		if (currentselection == 9) print_string(text, TextRed, 0, 5, 185+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 185+2, backbuffer->pixels);

		snprintf(text, sizeof(text), "R      : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][9]));
		if (currentselection == 10) print_string(text, TextRed, 0, 165, 25+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 165, 25+2, backbuffer->pixels);

		snprintf(text, sizeof(text), "START  : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][10]));
		if (currentselection == 11) print_string(text, TextRed, 0, 165, 45+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 165, 45+2, backbuffer->pixels);

		snprintf(text, sizeof(text), "SELECT : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][11]));
		if (currentselection == 12) print_string(text, TextRed, 0, 165, 65+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 165, 65+2, backbuffer->pixels);

		Update_Video_Menu();
	}

	config_save();
}

void Menu()
{
	char text[50];
    int16_t pressed = 0;
    int16_t currentselection = 1;
    SDL_Rect dstRect;
    SDL_Event Event;

    Set_Video_Menu();

	/* Save sram settings each time we bring up the menu */
	SRAM_Menu(0);
	
    while (((currentselection != 1) && (currentselection != 6)) || (!pressed))
    {
        pressed = 0;

        SDL_FillRect( backbuffer, NULL, 0 );

		print_string("SNESEmu - Built on " __DATE__, TextWhite, 0, 5, 15, backbuffer->pixels);

		if (currentselection == 1) print_string("Continue", TextRed, 0, 5, 45, backbuffer->pixels);
		else  print_string("Continue", TextWhite, 0, 5, 45, backbuffer->pixels);

		snprintf(text, sizeof(text), "Load State %d", save_slot);

		if (currentselection == 2) print_string(text, TextRed, 0, 5, 65, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 65, backbuffer->pixels);

		snprintf(text, sizeof(text), "Save State %d", save_slot);

		if (currentselection == 3) print_string(text, TextRed, 0, 5, 85, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 85, backbuffer->pixels);

        if (currentselection == 4)
        {
			switch(option.fullscreen)
			{
				case 0:
					print_string("Scaling : Native", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
				case 1:
					print_string("Scaling : Stretched", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
				case 2:
					print_string("Scaling : Bilinear", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
				case 3:
					print_string("Scaling : EPX/Scale2x", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
			}
        }
        else
        {
			switch(option.fullscreen)
			{
				case 0:
					print_string("Scaling : Native", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
				case 1:
					print_string("Scaling : Stretched", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
				case 2:
					print_string("Scaling : Bilinear", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
				case 3:
					print_string("Scaling : EPX/Scale2x", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
			}
        }

		if (currentselection == 5) print_string("Input remapping", TextRed, 0, 5, 125, backbuffer->pixels);
		else print_string("Input remapping", TextWhite, 0, 5, 125, backbuffer->pixels);

		if (currentselection == 6) print_string("Quit", TextRed, 0, 5, 145, backbuffer->pixels);
		else print_string("Quit", TextWhite, 0, 5, 145, backbuffer->pixels);

		print_string("Frontend by gameblabla", TextWhite, 0, 5, 205, backbuffer->pixels);
		print_string("Credits: Snes9x dev team, libretro", TextWhite, 0, 5, 225, backbuffer->pixels);

        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_KEYDOWN)
            {
                switch(Event.key.keysym.sym)
                {
                    case BTN_UP:
                        currentselection--;
                        if (currentselection == 0)
                            currentselection = 6;
                        break;
                    case BTN_DOWN:
                        currentselection++;
                        if (currentselection == 7)
                            currentselection = 1;
                        break;
                    case BTN_L2:
                    case BTN_SELECT:
                    case BTN_B:
						pressed = 1;
						currentselection = 1;
						break;
                    case BTN_A:
                    case BTN_START:
                        pressed = 1;
                        break;
                    case BTN_LEFT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                if (save_slot > 0) save_slot--;
							break;
                            case 4:
							option.fullscreen--;
							if (option.fullscreen < 0)
								option.fullscreen = upscalers_available;
							break;
                        }
                        break;
                    case BTN_RIGHT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                save_slot++;
								if (save_slot == 10)
									save_slot = 9;
							break;
                            case 4:
                                option.fullscreen++;
                                if (option.fullscreen > upscalers_available)
                                    option.fullscreen = 0;
							break;
                        }
                        break;
					default:
					break;
                }
            }
            else if (Event.type == SDL_QUIT)
            {
				currentselection = 6;
				pressed = 1;
			}
        }

        if (pressed)
        {
            switch(currentselection)
            {
				case 5:
					Input_Remapping();
				break;
                case 4 :
                    option.fullscreen++;
                    if (option.fullscreen > upscalers_available)
                        option.fullscreen = 0;
                    break;
                case 2 :
                    SaveState_Menu(1, save_slot);
					currentselection = 1;
                    break;
                case 3 :
					SaveState_Menu(0, save_slot);
					currentselection = 1;
				break;
				default:
				break;
            }
        }

		Update_Video_Menu();
    }

    SDL_FillRect(sdl_screen, NULL, 0);
    SDL_Flip(sdl_screen);
    #ifdef SDL_TRIPLEBUF
    SDL_FillRect(sdl_screen, NULL, 0);
    SDL_Flip(sdl_screen);
    #endif

    if (currentselection == 6)
    {
        exit_snes = 1;
	}

	/* Switch back to emulator core */
	config_save();
	emulator_state = 0;
	Set_Video_InGame();
}

static void Cleanup(void)
{
#ifdef SCALE2X_UPSCALER
	if (scale2x_buf) SDL_FreeSurface(scale2x_buf);
#endif
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
	if (backbuffer) SDL_FreeSurface(backbuffer);

	// Deinitialize audio and video output
	Audio_Close();

	SDL_Quit();
}

void Init_Configuration()
{
	snprintf(home_path, sizeof(home_path), "%s/.snes9x2002", getenv("HOME"));

	snprintf(conf_path, sizeof(conf_path), "%s/conf", home_path);
	snprintf(save_path, sizeof(save_path), "%s/sstates", home_path);
	snprintf(sram_path, sizeof(sram_path), "%s/sram", home_path);
	snprintf(rtc_path, sizeof(sram_path), "%s/rtc", home_path);

	/* We check first if folder does not exist.
	 * Let's only try to create it if so in order to decrease boot times.
	 * */

	if (access( home_path, F_OK ) == -1)
	{
		mkdir(home_path, 0755);
	}

	if (access( save_path, F_OK ) == -1)
	{
		mkdir(save_path, 0755);
	}

	if (access( conf_path, F_OK ) == -1)
	{
		mkdir(conf_path, 0755);
	}

	if (access( sram_path, F_OK ) == -1)
	{
		mkdir(sram_path, 0755);
	}

	if (access( rtc_path, F_OK ) == -1)
	{
		mkdir(rtc_path, 0755);
	}

	/* Load sram file if it exists */
	SRAM_Menu(1);
	
	config_load();
}

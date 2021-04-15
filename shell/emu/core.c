// Author: skogaby
// Lots of examples were followed using other emulators found on github

#include <SDL/SDL.h>
#include <libgen.h>
#include <sys/time.h>
#include <sys/types.h>

#include "main.h"
#include "snes9x.h"
#include "soundux.h"
#include "memmap.h"
#include "apu.h"
#ifdef USE_BLARGG_APU
#include "apu_blargg.h"
#endif
#include "cheats.h"
#include "display.h"
#include "gfx.h"
#include "cpuexec.h"
#ifdef USE_SPC7110
#include "spc7110.h"
#endif
#include "srtc.h"
#include "sa1.h"
#include "snapshot.h"
#include "scaler.h"

#include "shared.h"
#include "menu.h"
#include "video_blit.h"
#include "input.h"
#include "sound_output.h"

#include <dlfcn.h>
#include <mmenu.h>
static void* mmenu = NULL;
static char rom_path[512];
static char save_path[512];
static int resume_slot = -1;

char GameName_emu[512];

bool overclock_cycles = false;
bool reduce_sprite_flicker = true;
int one_c, slow_one_c, two_c;

static int32_t samples_to_play = 0;
static int32_t samples_per_frame = 0;
static int32_t samplerate = (((SNES_CLOCK_SPEED * 6) / (32 * ONE_APU_CYCLE)));

#ifdef USE_BLARGG_APU
static void S9xAudioCallback()
{
   size_t avail;
   /* Just pick a big buffer. We won't use it all. */
   static int16_t audio_buf[0x20000];

   S9xFinalizeSamples();
   avail = S9xGetSampleCount();
   S9xMixSamples(audio_buf, avail);
   Audio_Write(audio_buf, avail >> 1);
}
#endif

bool8 S9xInitUpdate() { return TRUE; }
bool8 S9xDeinitUpdate(int width, int height, bool8_32 sixteen_bit)
{
	if (IPPU.RenderThisFrame) {
		Update_Video_Ingame(width, height);
	}
}

void SRAM_Save(char* path, uint_fast8_t state)
{
	FILE* savefp;
	
	/* If SRAM is not used then don't bother saving any SRAM file */
	if (CPU.Memory_SRAMMask == 0) return;

	if (state == 1)
	{
		savefp = fopen(path, "rb");
		if (savefp)
		{
			fread(Memory.SRAM, sizeof(uint8_t), CPU.Memory_SRAMMask, savefp);
			fclose(savefp);
		}
	}
	else
	{
		savefp = fopen(path, "wb");
		if (savefp)
		{
			fwrite(Memory.SRAM, sizeof(uint8_t), CPU.Memory_SRAMMask, savefp);
			fclose(savefp);
		}
	}
}

void SaveState(char* path, uint_fast8_t state)
{
	if (state == 1)
	{
		S9xUnfreezeGame(path);
	}
	else
	{
		S9xFreezeGame(path);
	}
}


#ifdef FRAMESKIP
static uint32_t Timer_Read(void) 
{
	/* Timing. */
	struct timeval tval;
  	gettimeofday(&tval, 0);
	return (((tval.tv_sec*1000000) + (tval.tv_usec)));
}
static long lastTick = 0, newTick;
static uint32_t SkipCnt = 0, video_frames = 0, FPS = 60, FrameSkip;
static const uint32_t TblSkip[4][4] = {
    {0, 0, 0, 0},
    {0, 0, 0, 1},
	{0, 0, 1, 1},
	{0, 1, 1, 1}
};
#endif

#ifdef AUDIO_FRAMESKIP
#define MAX_SKIP_COUNT 4
static uint32_t SkipCnt = 0;
#endif

void Emulation_Run (void)
{
#ifndef USE_BLARGG_APU
	static int16_t audio_buf[2048];
#endif

#ifdef FRAMESKIP
	SkipCnt++;
	if (SkipCnt > 3) SkipCnt = 0;
	if (TblSkip[FrameSkip][SkipCnt]) IPPU.RenderThisFrame = false;
	else IPPU.RenderThisFrame = true;
#else
	IPPU.RenderThisFrame = true;
#endif
	
#ifdef USE_BLARGG_APU
	S9xSetSoundMute(false);
#endif
	//Settings.HardDisableAudio = false;

#ifdef AUDIO_FRAMESKIP
	if (IPPU.RenderThisFrame) {
		if (Audio_Underrun_Likely()) {
			if (SkipCnt < MAX_SKIP_COUNT) {
				SkipCnt++;
				IPPU.RenderThisFrame = false;
			} else {
				SkipCnt = 0;
			}
		}
	} else {
		SkipCnt = 0;
	}
#endif

	S9xMainLoop();

#ifndef USE_BLARGG_APU
	samples_to_play += samples_per_frame;

	if (samples_to_play > 512)
	{
		S9xMixSamples(audio_buf, samples_to_play);
		Audio_Write(audio_buf, samples_to_play / 2);
		samples_to_play = 0;
	}
#else
	S9xAudioCallback();
#endif

#ifdef FRAMESKIP
	video_frames++;
	newTick = Timer_Read();
	if ( (newTick) - (lastTick) > 1000000) 
	{
		FPS = video_frames;
		video_frames = 0;
		lastTick = newTick;
		if (FPS >= 60)
		{
			FrameSkip = 0;
		}
		else
		{
			FrameSkip = 60 / FPS;
			if (FrameSkip > 3) FrameSkip = 3;
		}
	}
#endif
}


bool Load_Game_Memory(char* game_path)
{
	uint64_t fps;
	CPU.Flags = 0;

	if (!LoadROM(game_path))
		return false;

	Settings.FrameTime = (Settings.PAL ? Settings.FrameTimePAL : Settings.FrameTimeNTSC);
	
   if (!Settings.PAL)
      fps = (SNES_CLOCK_SPEED * 6.0 / (SNES_CYCLES_PER_SCANLINE * SNES_MAX_NTSC_VCOUNTER));
   else
      fps = (SNES_CLOCK_SPEED * 6.0 / (SNES_CYCLES_PER_SCANLINE * SNES_MAX_PAL_VCOUNTER));
      
	samplerate = SOUND_OUTPUT_FREQUENCY;
	Settings.SoundPlaybackRate = samplerate;
   
#ifndef USE_BLARGG_APU
	Settings.SixteenBitSound = true;
	so.stereo = Settings.Stereo;
	so.playback_rate = Settings.SoundPlaybackRate;
	S9xSetPlaybackRate(so.playback_rate);

	samples_per_frame = samplerate / fps << 1;
#endif
	return true;
}


void init_sfc_setting(void)
{
   memset(&Settings, 0, sizeof(Settings));
   Settings.JoystickEnabled = false;
   Settings.SoundPlaybackRate = samplerate;
   Settings.Stereo = true;
   Settings.SoundBufferSize = 0;
   Settings.CyclesPercentage = 100;

   Settings.DisableSoundEcho = false;
   Settings.InterpolatedSound = true;
   Settings.APUEnabled = true;

   Settings.H_Max = SNES_CYCLES_PER_SCANLINE;
   Settings.SkipFrames = AUTO_FRAMERATE;
   Settings.FrameTimePAL = 20000;
   Settings.FrameTimeNTSC = 16667;
   Settings.Shutdown = Settings.ShutdownMaster = true;
   Settings.DisableSampleCaching = false;
   Settings.DisableMasterVolume = false;
   Settings.Mouse = false;
   Settings.SuperScope = false;
   Settings.MultiPlayer5 = false;
   Settings.ControllerOption = SNES_JOYPAD;
#ifdef USE_BLARGG_APU
   Settings.SoundSync = true;
#endif

   Settings.ForceTransparency = false;
   Settings.Transparency = true;
   Settings.SixteenBit = true;
   Settings.SupportHiRes = false;
   Settings.AutoSaveDelay = 30;
   Settings.ApplyCheats = true;
   Settings.TurboMode = false;
   Settings.TurboSkipFrames = 15;
#ifdef ASM_SPC700
   Settings.asmspc700 = true;
#endif
   Settings.SpeedHacks = true;
   Settings.HBlankStart = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;
   Settings.DisplayFrameRate = false;
}

void Init_SFC(void)
{
   init_sfc_setting();
   MemoryInit();
   S9xInitAPU();
   S9xInitDisplay(NULL, NULL);
   S9xGraphicsInit();
#ifdef USE_BLARGG_APU
   S9xInitSound(1000, 0); /* just give it a 1 second buffer */
   S9xSetSamplesAvailableCallback(S9xAudioCallback);
#else
   S9xInitSound();
#endif
}

void Deinit_SFC(void)
{
#ifdef USE_SPC7110
   if (Settings.SPC7110)
      Del7110Gfx();
#endif

   S9xGraphicsDeinit();
   S9xDeinitDisplay();
   S9xDeinitAPU();
   MemoryDeinit();
}



/* Main entrypoint of the emulator */
int main(int argc, char* argv[])
{
	int isloaded;
	
	printf("Starting Snes9x2002\n");
    
	if (argc < 2)
	{
		printf("Specify a ROM to load in memory\n");
		return 0;
	}
	
	snprintf(GameName_emu, sizeof(GameName_emu), "%s", basename(argv[1]));
	Init_SFC();
	
	Init_Video();
	Audio_Init();
	
	overclock_cycles = false;
	one_c = 4;
	slow_one_c = 5;
	two_c = 6;
	reduce_sprite_flicker = false;
	
	isloaded = Load_Game_Memory(argv[1]);
	if (!isloaded)
	{
		printf("Could not load ROM in memory\n");
		return 0;
	}
	
	Init_Configuration();
	
	mmenu = dlopen("libmmenu.so", RTLD_LAZY);
	strcpy(rom_path, argv[1]);
	SaveState_PathTemplate(save_path, 512);
	if (mmenu) {
		ResumeSlot_t ResumeSlot = (ResumeSlot_t)dlsym(mmenu, "ResumeSlot");
		if (ResumeSlot) resume_slot = ResumeSlot();
	}
	
    // get the game ready
    while (!exit_snes)
    {
		if (resume_slot!=-1) {
			SaveState_Menu(1, resume_slot);
			resume_slot = -1;
		}
		
		switch(emulator_state)
		{
			case 0:
				Emulation_Run();
			break;
			case 1:
				if (mmenu) {
					SRAM_Menu(0);
					
					ShowMenu_t ShowMenu = (ShowMenu_t)dlsym(mmenu, "ShowMenu");
					MenuReturnStatus status = ShowMenu(rom_path, save_path, sdl_screen, kMenuEventKeyDown);

					if (status==kStatusExitGame) {
						exit_snes = 1;
					}
					else if (status==kStatusOpenMenu) {
						Menu();
					}
					else if (status>=kStatusLoadSlot) {
						int slot = status - kStatusLoadSlot;
						SaveState_Menu(1, slot);
					}
					else if (status>=kStatusSaveSlot) {
						int slot = status - kStatusSaveSlot;
						SaveState_Menu(0, slot);
					}
					emulator_state = 0;
					SDL_FillRect(sdl_screen, NULL, 0);
				}
				else {
					Menu();
				}
			break;
		}
    }
    
	Deinit_SFC();
    Audio_Close();
    Video_Close();

    return 0;
}

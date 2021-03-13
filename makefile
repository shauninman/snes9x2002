GIT_VERSION := "$(shell git describe --abbrev=7 --dirty --always)"

PRGNAME = snes9x2002

VIDEO_BACKEND = sdl
INPUT_BACKEND = sdl
SOUND_BACKEND = sdl

PREFIX  = /opt/trimui-toolchain/bin/arm-buildroot-linux-gnueabi

# define regarding OS, which compiler to use
CC	= $(PREFIX)-gcc
STRIP	= $(PREFIX)-strip
AS	= $(PREFIX)-as
GASM	= $(PREFIX)-g++

SYSROOT := $(shell $(CC) --print-sysroot)
SDL_CFLAGS := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

# change compilation / linking flag options
CFLAGS	= -DLSB_FIRST -DFAST_ALIGNED_LSB_WORD_ACCESS -I. -Ilibretro/libretro-common/include -Isrc -DRIGHTSHIFT_IS_SAR
CFLAGS	+= -I./shell/emu -I./shell/scalers -I./shell/emu -I./shell/audio -I./shell/menu -I./shell/video/sdl -I./shell/input -Ishell/headers

CFLAGS	+= -Ofast -fsingle-precision-constant -fno-PIC -flto
ifndef PROFILE
CFLAGS	+= -falign-functions=1 -falign-jumps=1 -falign-loops=1 -falign-labels=1
endif
CFLAGS	+= -mcpu=arm926ej-s -mtune=arm926ej-s
CFLAGS	+= -DNDEBUG -DAUDIO_FRAMESKIP -DGIT_VERSION=\"$(GIT_VERSION)\" -DTRIMUI -fno-builtin -fno-exceptions -ffunction-sections -std=gnu99
CFLAGS	+= -Wall -Wextra -pedantic -Wno-implicit-function-declaration -Wno-implicit-fallthrough -Wno-sign-compare -Wno-unused-variable -Wno-unused-function -Wno-uninitialized -Wno-strict-aliasing -Wno-overflow -fno-strict-overflow

ifeq ($(PROFILE), YES)
CFLAGS	+= -fprofile-generate=./profile
else ifeq ($(PROFILE), APPLY)
CFLAGS	+= -fprofile-use -fprofile-dir=./profile -fbranch-probabilities
endif

LDFLAGS = -lc -lgcc -lm $(SDL_LIBS) -no-pie -Wl,--gc-sections -s -flto
LDFLAGS += -lSDL_image -lSDL_ttf -ldl
ifeq ($(SOUND_BACKEND), portaudio)
LDFLAGS	+= -lasound -lportaudio
endif
ifeq ($(SOUND_BACKEND), libao)
LDFLAGS	+= -lao
endif
ifeq ($(SOUND_BACKEND), alsa)
LDFLAGS	+= -lasound
endif
ifeq ($(SOUND_BACKEND), pulse)
LDFLAGS	+= -lpulse -lpulse-simple
endif

ifeq ($(PROFILE), YES)
LDFLAGS	+= -lgcov
endif

CORE_DIR         := ./src
LIBRETRO_DIR     := ./libretro

LAGFIX = 1
ARM_ASM = 1
ASM_CPU = 0
ASM_SPC700 = 0

include Makefile.common

# Files to be compiled
SRCDIR    = ./shell/emu ./shell/scalers ./shell/audio/$(SOUND_BACKEND) ./shell/menu ./shell/video/$(VIDEO_BACKEND) ./shell/input/$(INPUT_BACKEND) ./shell/other
VPATH     = $(SRCDIR)
SRC_C   = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
SRC_CP   = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.cpp))
OBJ_C   = $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJ_CP   = $(notdir $(patsubst %.cpp, %.o, $(SRC_CP)))
OBJS     = $(SOURCES:.c=.o) $(SOURCES_ASM:.S=.o) $(OBJ_C) $(OBJ_CP)

CFLAGS += $(DEFINES) $(COMMON_DEFINES) $(INCLUDES)

# Rules to make executable
$(PRGNAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(PRGNAME) $^ $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -Wa,-I./src/ -c -o $(OBJOUT)$@ $<

clean:
	rm -f $(PRGNAME)$(EXESUFFIX) *.o src/*.o

#ifndef SOUND_OUTPUT_H
#define SOUND_OUTPUT_H

#include <stdbool.h>
#include "shared.h"

extern uint32_t Audio_Init();
extern void Audio_Write(int16_t* restrict buffer, uint32_t buffer_size);
extern bool Audio_Underrun_Likely();
extern void Audio_Close();

#endif

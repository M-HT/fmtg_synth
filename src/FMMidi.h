/**
 *
 *  Copyright (C) 2025 Roman Pauer
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of
 *  this software and associated documentation files (the "Software"), to deal in
 *  the Software without restriction, including without limitation the rights to
 *  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *  of the Software, and to permit persons to whom the Software is furnished to do
 *  so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */

#if !defined(_FMMIDI_H_INCLUDED_)
#define _FMMIDI_H_INCLUDED_

#include <stdint.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define RESTRICT restrict
#elif defined(__GNUC__) || defined(__llvm__)
    #define RESTRICT __restrict__
#elif defined(_MSC_VER)
    #define RESTRICT __restrict
#else
    #undef RESTRICT
#endif


#ifdef __cplusplus
extern "C" {
#endif

unsigned int FM_GetSamplingFrequency(void);
unsigned int FM_GetRenderedSamplesPerCall(void);

void FM_CreateChannelManager(int num_voices, int num_threads);
void FM_SetPSG(int PSG);
int FM_AddToneData(const uint8_t *tonedata);

void FM_MidiMessageShort(uint32_t message);
void FM_MidiMessageLong(const uint8_t *message, unsigned int length);

void FM_RenderSamplesS16Interleaved(int16_t *samples);
void FM_RenderSamplesFloat(float * RESTRICT samples0, float * RESTRICT samples1);


#ifdef __cplusplus
}
#endif

#endif /* _FMMIDI_H_INCLUDED_ */


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

#include "FMMidi.h"
#include "ChannelManager.h"
#include "Channel8.h"
#include <string.h>
#ifdef MULTI_THREAD
    #include <stdlib.h>
#ifdef _WIN32
    #include <process.h>
    typedef HANDLE fm_semaphore_t;
#elif defined(__APPLE__)
    #include <sys/sysctl.h>
    #include <dispatch/dispatch.h>
    typedef dispatch_semaphore_t fm_semaphore_t;
#else
    #include <semaphore.h>
    #include <limits.h>
    #include <unistd.h>
    typedef sem_t *fm_semaphore_t;
#endif
    #ifndef PTHREAD_STACK_MIN
        #define PTHREAD_STACK_MIN 16384
    #endif
#endif

#ifdef __AVX2__
    #define ALIGN_BUF 32
    #include <immintrin.h>
#elif defined(__SSE2__)
    #define ALIGN_BUF 16
    #include <emmintrin.h>
#elif defined(__ARM_NEON__)
    #define ALIGN_BUF 16
    #include <arm_neon.h>
#endif


#define MIDI_UNIT			256


struct Drum {
	u8 tn, sc, pan, alt;
};

struct Channel {
	Channel() { Reset(); }
	void Reset() {
		prognum = 0;
		volume = 100;
		pan = 64;
		expression = rpnl = rpnm = 127;
		bend = 0;
		bendsen = 2;
		percid = 0;
	}
	u8 volume, rpnl, rpnm, pan, expression, prognum;
	s16 bend;
	s8 bendsen;
	u8 percid;
};

class FM_ChannelManager : ChannelManager {
public:
	void NotesOff(u8 id_low);
	void SoundsOff(u8 id_low);
	void SoundsOff(void);
	static Drum &GetDrum(u8 note) { return drumData[drumBank][note]; }
};


static Channel channels[16];
#ifdef MULTI_THREAD
static struct thread_info_t
{
    s32 *buf;
    volatile int quit;
    fm_semaphore_t sem1, sem2;
#ifdef _WIN32
    HANDLE thread;
#elif defined(__APPLE__)
#else
    pthread_t thread;
#endif
} *thread_info;
static int ncore;
#endif


unsigned int FM_GetSamplingFrequency(void)
{
    return FS;
}

unsigned int FM_GetRenderedSamplesPerCall(void)
{
    return MIDI_UNIT;
}


#ifdef MULTI_THREAD
#ifdef _WIN32
int pthread_mutex_init(pthread_mutex_t *mutex, void *mutexattr)
{
    InitializeCriticalSection(mutex);
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    DeleteCriticalSection(mutex);
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    EnterCriticalSection(mutex);
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    LeaveCriticalSection(mutex);
    return 0;
}
#endif

static inline int fm_semaphore_create(fm_semaphore_t *sem)
{
    fm_semaphore_t _sem;
#ifdef _WIN32
    _sem = CreateSemaphoreW(NULL, 0, 1, NULL);
#elif defined(__APPLE__)
    _sem = dispatch_semaphore_create(0);
#else
    _sem = new sem_t;
    if (0 != sem_init(_sem, 0, 0))
    {
        delete _sem;
        _sem = NULL;
    }
#endif
    *sem = _sem;
    return (_sem != NULL) ? 1 : 0;
}

static inline void fm_semaphore_destroy(fm_semaphore_t sem)
{
#ifdef _WIN32
    CloseHandle(sem);
#elif defined(__APPLE__)
    dispatch_release(sem);
#else
    sem_destroy(sem);
    delete sem;
#endif
}

static inline void fm_semaphore_wait(fm_semaphore_t sem)
{
#ifdef _WIN32
    while (WaitForSingleObject(sem, INFINITE));
#elif defined(__APPLE__)
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
#else
    while (sem_wait(sem));
#endif
}

static inline void fm_semaphore_signal(fm_semaphore_t sem)
{
#ifdef _WIN32
    ReleaseSemaphore(sem, 1, NULL);
#elif defined(__APPLE__)
    dispatch_semaphore_signal(sem);
#else
    sem_post(sem);
#endif
}

static
#ifdef _WIN32
unsigned __stdcall
#elif defined(__APPLE__)
void
#else
void *
#endif
RenderThread(void *arg)
{
    thread_info_t *info = (thread_info_t *)arg;

    for (;;)
    {
        fm_semaphore_wait(info->sem1);
        if (info->quit)
        {
            fm_semaphore_signal(info->sem2);
            break;
        }
        memset(info->buf, 0, sizeof(s32) * 2 * MIDI_UNIT);
        while (!gMan->Update1(info->buf, MIDI_UNIT));
        fm_semaphore_signal(info->sem2);
    }
#ifdef _WIN32
    return 0;
#elif defined(__APPLE__)
    return;
#else
    return NULL;
#endif
}

static void DestroyThreads(void)
{
    if (ncore > 1)
    {
        int num_threads = ncore - 1;

        for (int i = num_threads; i != 0; i--)
        {
            thread_info[i - 1].quit = 1;
            fm_semaphore_signal(thread_info[i - 1].sem1);
        }

        for (int i = num_threads; i != 0; i--)
        {
            fm_semaphore_wait(thread_info[i - 1].sem2);
#ifdef ALIGN_BUF
            delete[] (s32 *)(((uintptr_t)thread_info[i - 1].buf) - thread_info[i - 1].buf[-1]);
#else
            delete[] thread_info[i - 1].buf;
#endif
#ifdef _WIN32
            CloseHandle(thread_info[i - 1].thread);
#endif
            fm_semaphore_destroy(thread_info[i - 1].sem2);
            fm_semaphore_destroy(thread_info[i - 1].sem1);
        }

        delete[] thread_info;

        ncore = 1;
    }
}

static void CreateThreads(int num_threads)
{
#ifdef ALIGN_BUF
    s32 *unaligned_buf;
#endif

#ifdef _WIN32
    SYSTEM_INFO si;

    GetSystemInfo(&si);
    if (si.dwNumberOfProcessors > 0 && (unsigned int)num_threads > si.dwNumberOfProcessors) num_threads = si.dwNumberOfProcessors;
#elif defined(__APPLE__)
    int mib[2], num_procs;
    size_t len;
    dispatch_queue_global_t queue;

    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    len = sizeof(num_procs);
    if (0 == sysctl(mib, 2, &num_procs, &len, NULL, 0))
    {
        if (num_procs > 0 && num_threads > num_procs) num_threads = num_procs;
    }
#else
    long num_procs;
    pthread_attr_t attr;

    num_procs = sysconf(_SC_NPROCESSORS_CONF);
    if (num_procs > 0 && num_threads > num_procs) num_threads = num_procs;
#endif

    ncore = 1;
    if (num_threads <= 1) return;

#ifdef _WIN32
#elif defined(__APPLE__)
    queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
#else
    if (0 != pthread_attr_init(&attr)) return;

    if (0 != pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
    {
        pthread_attr_destroy(&attr);
        return;
    }

    if (0 != pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN + 16384))
    {
        pthread_attr_destroy(&attr);
        return;
    }
#endif

    thread_info = new thread_info_t[num_threads - 1];

    for (int i = 0; i < num_threads - 1; i++)
    {
        if (!fm_semaphore_create(&thread_info[i].sem1)) break;
        if (!fm_semaphore_create(&thread_info[i].sem2))
        {
            fm_semaphore_destroy(thread_info[i].sem1);
            break;
        }

#ifdef ALIGN_BUF
        unaligned_buf = new s32[MIDI_UNIT * 2 + ((2 * ALIGN_BUF) / sizeof(s32))];
        thread_info[i].buf = (s32 *) ((2 * ALIGN_BUF + (uintptr_t)unaligned_buf) & ~((uintptr_t) (2 * ALIGN_BUF) - 1));
        thread_info[i].buf[-1] = (s32)(((uintptr_t)thread_info[i].buf) - ((uintptr_t)unaligned_buf));
#else
        thread_info[i].buf = new s32[MIDI_UNIT * 2];
#endif

        thread_info[i].quit = 0;

#ifdef _WIN32
        thread_info[i].thread = (HANDLE)_beginthreadex(NULL, PTHREAD_STACK_MIN + 16384, &RenderThread, &thread_info[i], 0, NULL);
        if (NULL == thread_info[i].thread)
#elif defined(__APPLE__)
        dispatch_async_f(queue, &thread_info[i], &RenderThread);
        if (0)
#else
        if (0 != pthread_create(&thread_info[i].thread, &attr, &RenderThread, &thread_info[i]))
#endif
        {
#ifdef ALIGN_BUF
            delete[] unaligned_buf;
#else
            delete[] thread_info[i].buf;
#endif
            fm_semaphore_destroy(thread_info[i].sem2);
            fm_semaphore_destroy(thread_info[i].sem1);
            break;
        }

        ncore++;
    }

#ifdef _WIN32
#elif defined(__APPLE__)
#else
    pthread_attr_destroy(&attr);
#endif

    if (ncore == 1)
    {
        delete[] thread_info;
    }
    else
    {
        atexit(&DestroyThreads);
    }
}
#endif

void FM_CreateChannelManager(int num_voices, int num_threads)
{
    if (!gMan)
    {
        Operator8::MakeTable();
        gMan = new ChannelManager(num_voices);
#ifdef MULTI_THREAD
        CreateThreads(num_threads);
#endif
    }
}

void FM_SetPSG(int PSG)
{
    Channel8::SetModulate(!PSG);
    Operator8::SetWave(PSG);
}

int FM_AddToneData(const uint8_t *tonedata)
{
    if (!tonedata) return 1;
    if (tonedata[0] != 'T' || tonedata[1] != 'O' || tonedata[2] != 'N' || tonedata[3] != 'E') return 1;

    tonedata += 4;

    Channel8::AppendToneData(&tonedata[tonedata[0] | (tonedata[1] << 8)]);
    ChannelManager::AppendDrumData((Drum *)&tonedata[tonedata[2] | (tonedata[3] << 8)]);

    return 0;
}


void FM_ChannelManager::NotesOff(u8 id_low) {
	for (ChannelList::iterator i = active.begin(); i != active.end(); ++i)
		if (((*i)->getId() & 0xff) == id_low)
			(*i)->NoteOff(true);
}

void FM_ChannelManager::SoundsOff(u8 id_low) {
	for (ChannelList::iterator i = active.begin(); i != active.end();)
		if (((*i)->getId() & 0xff) == id_low) {
			delete *i;
			i = active.erase(i);
		} else ++i;
}

void FM_ChannelManager::SoundsOff(void) {
	for (ChannelList::iterator i = active.begin(); i != active.end();) {
		delete *i;
		i = active.erase(i);
	}
}


void FM_MidiMessageShort(uint32_t message)
{
    unsigned int chnum = message & 0x0f;
    Channel *pchannel = &channels[chnum];

    switch ((message >> 4) & 0x0f)
    {
        case 0x09: // Note on
            if (u8 velocity = (message >> 16) & 0x7f)
            {
                u8 prognum = pchannel->prognum;
                u8 note = (message >> 8) & 0x7f;
                u8 pan = pchannel->pan;

                if (pchannel->percid && chnum != 10 - 1)
                {
                    Drum &d = FM_ChannelManager::GetDrum(note);
                    if (!d.tn) break;
                    prognum = d.tn;
                    note = d.sc;
                    pan = d.pan;
                }

                gMan->KeyOn(prognum, note, velocity, pchannel->volume, pchannel->expression, pan, ((s32)pchannel->bendsen * pchannel->bend) >> 6, (message & 0x7f00) | (chnum + pchannel->percid));
                break;
            }
            // fallthrough
        case 0x08: // Note off
            gMan->KeyOff((message & 0x7f00) | (chnum + pchannel->percid));
            break;
        case 0x0B: // Controller change
            switch ((message >> 8) & 0x7f)
            {
                case 7: // Volume
                    pchannel->volume = (message >> 16) & 0x7f;
                    gMan->SetVolExp(chnum + pchannel->percid, pchannel->volume, pchannel->expression);
                    break;
                case 10: // Pan
                    pchannel->pan = (message >> 16) & 0x7f;
                    gMan->SetPan(chnum + pchannel->percid, pchannel->pan);
                    break;
                case 11: // Expression
                    pchannel->expression = (message >> 16) & 0x7f;
                    gMan->SetVolExp(chnum + pchannel->percid, pchannel->volume, pchannel->expression);
                    break;
                case 98: // NRPN LSB
                case 99: // NRPN MSB
                    pchannel->rpnl = pchannel->rpnm = 127;
                    break;
                case 100: // RPN LSB
                    pchannel->rpnl = (message >> 16) & 0x7f;
                    break;
                case 101: // RPN MSB
                    pchannel->rpnm = (message >> 16) & 0x7f;
                    break;
                case 6: // Data entry MSB
                    if ((pchannel->rpnl == 0) && (pchannel->rpnm == 0))
                    {
                        pchannel->bendsen = (message >> 16) & 0x1f;
                        //pchannel->rpnl = pchannel->rpnm = 127;
                    }
                    break;
                case 121: // Reset all controllers
                    pchannel->expression = pchannel->rpnl = pchannel->rpnm = 127;
                    pchannel->bend = 0;
                    pchannel->bendsen = 2;
                    gMan->SetVolExp(chnum + pchannel->percid, pchannel->volume, pchannel->expression);
                    gMan->Bend(chnum + pchannel->percid, 0);
                    break;
                case 123: // All notes off
                    reinterpret_cast<FM_ChannelManager *>(gMan)->NotesOff(chnum + pchannel->percid);
                    break;
                case 120: // All sounds off
                case 124: // Omni off
                case 125: // Omni on
                case 126: // Mono on
                case 127: // Poly on
                    reinterpret_cast<FM_ChannelManager *>(gMan)->SoundsOff(chnum + pchannel->percid);
                    break;
                default:
                    break;
            }
            break;
        case 0x0C: // Program change
            pchannel->prognum = (message >> 8) & 0x7f;
            break;
        case 0x0E: // Pitch bend
            pchannel->bend = (((message >> 9) & 0x3f80) | ((message >> 8) & 0x7f)) - 8192;
            gMan->Bend(chnum + pchannel->percid, ((s32)pchannel->bendsen * pchannel->bend) >> 6);
            break;
        case 0x0A: // Pressure change
        case 0x0D: // Channel pressure
        default:
            break;
    }

}

static void GMReset(void)
{
    reinterpret_cast<FM_ChannelManager *>(gMan)->SoundsOff();

    for (int i = 0; i < 16; i++)
    {
        channels[i].Reset();
    }
}

static void SetChannelPerc(uint8_t chnum, uint8_t perc)
{
    reinterpret_cast<FM_ChannelManager *>(gMan)->SoundsOff(chnum + channels[chnum].percid);
    channels[chnum].percid = (chnum != 10 - 1) ? ((perc) ? 0x10 : 0) : ((perc) ? 0 : 0x10);
}

void FM_MidiMessageLong(const uint8_t *message, unsigned int length)
{
    // do nothing

    if ((message == NULL) || (length == 0)) return;

    if ((message[0] != 0xf0) || (message[length - 1] != 0xf7)) return;

    if (message[1] == 0x7e)
    {
        // Universal Non-Real Time
        if ((message[2] == 0x7f) && (message[3] == 0x09) && (message[4] == 0x01) && (length == 6))
        {
            // General MIDI reset
            GMReset();
        }
    }
    else if (message[1] == 0x41)
    {
        // Roland Corporation
        if ((message[3] == 0x42) && (message[4] == 0x12) && (length == 11))
        {
            if ((message[5] == 0x40) && (message[6] == 0x00) && (message[7] == 0x7f) && (message[8] == 0x00))
            {
                // GS reset
                GMReset();
            }
            else if ((message[5] == 0x00) && (message[6] == 0x00) && (message[7] == 0x7f) && ((message[8] == 0x00) || (message[8] == 0x01)))
            {
                // GS mode 1/2
                GMReset();
            }
            else if ((message[5] == 0x40) && ((message[6] & 0xf0) == 0x10) && (message[7] == 0x15))
            {
                // Part to rhythm allocation
                uint8_t part = message[6] & 0x0f;
                SetChannelPerc((part == 0) ? 10 - 1 : (part < 10) ? part - 1 : part, message[8]);
            }
        }
    }
    else if (message[1] == 0x43)
    {
        // Yamaha Corporation
        if (((message[2] & 0x10) == 0x10) && (message[3] == 0x4c) && (length == 9))
        {
            if ((message[4] == 0x00) && (message[5] == 0x00) && (message[6] == 0x7e))
            {
                // XG reset
                GMReset();
            }
            else if ((message[4] == 0x08) && ((message[5] & 0xf0) == 0x00) && (message[6] == 0x07))
            {
                // Part to rhythm allocation
                SetChannelPerc(message[5], message[7]);
            }
        }
    }
}


void FM_RenderSamplesS16Interleaved(int16_t *samples)
{
#if defined(ALIGN_BUF)
#if __cplusplus >= 201103L || _MSVC_LANG >= 201103L
    alignas(ALIGN_BUF)
#elif defined(__GNUC__)
    __attribute__ ((aligned (ALIGN_BUF)))
#elif defined(_MSC_VER)
    __declspec(align(ALIGN_BUF))
#else
#error unaligned variable
#endif
#endif
    s32 buf[MIDI_UNIT * 2];

#ifdef MULTI_THREAD
    if (ncore > 1)
    {
        int num_threads = ncore - 1;

        gMan->StartEnum();

        for (int i = num_threads; i != 0; i--)
        {
            fm_semaphore_signal(thread_info[i - 1].sem1);
        }

        memset(buf, 0, sizeof(s32) * 2 * MIDI_UNIT);
        while (!gMan->Update1(buf, MIDI_UNIT));

        for (int i = num_threads; i != 0; i--)
        {
            fm_semaphore_wait(thread_info[i - 1].sem2);
        }

        gMan->Poll();

#ifdef __AVX2__
        for (int i = 0; i < MIDI_UNIT * 2; i += 16)
        {
            // buf is aligned
            __m256i mm1 = _mm256_load_si256((__m256i *) &buf[i]);       // mm1 = buf[i : i+7]
            __m256i mm2 = _mm256_load_si256((__m256i *) &buf[i + 8]);   // mm2 = buf[i+8 : i+15]
            for (int j = num_threads; j != 0; j--)
            {
                // thread_info[].buf is aligned
                __m256i mm1_2 = _mm256_load_si256((__m256i *) &thread_info[j - 1].buf[i]);      // mm1_2 = thread_info[j - 1].buf[i : i+7]
                __m256i mm2_2 = _mm256_load_si256((__m256i *) &thread_info[j - 1].buf[i + 8]);  // mm2_2 = thread_info[j - 1].buf[i+8 : i+15]
                mm1 = _mm256_add_epi32(mm1, mm1_2);
                mm2 = _mm256_add_epi32(mm2, mm2_2);
            }
            mm1 = _mm256_srai_epi32(mm1, 7);                            // mm1 = buf[i : i + 7] >> 7
            mm2 = _mm256_srai_epi32(mm2, 7);                            // mm2 = buf[i+8 : i+15] >> 7
            mm1 = _mm256_packs_epi32(mm1, mm2);                         // mm1 = SaturateSignedDoublewordToSignedWord(buf[i : i+3] >> 7, buf[i+8 : i+11] >> 7, buf[i+4 : i+7] >> 7, buf[i+12 : i+15] >> 7)
            mm1 = _mm256_permute4x64_epi64(mm1, _MM_SHUFFLE(3,1,2,0));  // mm1 = SaturateSignedDoublewordToSignedWord(buf[i : i+15] >> 7)
            _mm256_storeu_si256((__m256i *) &samples[i], mm1);          // samples[i : i+15] = SaturateSignedDoublewordToSignedWord(buf[i : i+15] >> 7)
        }
#elif defined(__SSE2__)
        for (int i = 0; i < MIDI_UNIT * 2; i += 8)
        {
            // buf is aligned
            __m128i mm1 = _mm_load_si128((__m128i *) &buf[i]);      // mm1 = buf[i : i+3]
            __m128i mm2 = _mm_load_si128((__m128i *) &buf[i + 4]);  // mm2 = buf[i+4 : i+7]
            for (int j = num_threads; j != 0; j--)
            {
                // thread_info[].buf is aligned
                __m128i mm1_2 = _mm_load_si128((__m128i *) &thread_info[j - 1].buf[i]);     // mm1_2 = thread_info[j - 1].buf[i : i+3]
                __m128i mm2_2 = _mm_load_si128((__m128i *) &thread_info[j - 1].buf[i + 4]); // mm2_2 = thread_info[j - 1].buf[i+4 : i+7]
                mm1 = _mm_add_epi32(mm1, mm1_2);
                mm2 = _mm_add_epi32(mm2, mm2_2);
            }
            mm1 = _mm_srai_epi32(mm1, 7);                           // mm1 = buf[i : i + 3] >> 7
            mm2 = _mm_srai_epi32(mm2, 7);                           // mm2 = buf[i+4 : i+7] >> 7
            mm1 = _mm_packs_epi32(mm1, mm2);                        // mm1 = SaturateSignedDoublewordToSignedWord(buf[i : i+7] >> 7)
            _mm_storeu_si128((__m128i *) &samples[i], mm1);         // samples[i : i+7] = SaturateSignedDoublewordToSignedWord(buf[i : i+7] >> 7)
        }
#elif defined(__ARM_NEON__)
        for (int i = 0; i < MIDI_UNIT * 2; i += 8)
        {
            int32x4_t n01 = vld1q_s32(&buf[i]);     // n01 = buf[i : i+3]
            int32x4_t n02 = vld1q_s32(&buf[i + 4]); // n02 = buf[i+4 : i+7]
            for (int j = num_threads; j != 0; j--)
            {
                // thread_info[].buf is aligned
                int32x4_t n01_2 = vld1q_s32(&thread_info[j - 1].buf[i]);        // n01_2 = thread_info[j - 1].buf[i : i+3]
                int32x4_t n02_2 = vld1q_s32(&thread_info[j - 1].buf[i + 4]);    // n02_2 = thread_info[j - 1].buf[i+4 : i+7]
                n01 = vaddq_s32(n01, n01_2);
                n02 = vaddq_s32(n02, n02_2);
            }
            int16x4_t n03 = vqshrn_n_s32(n01, 7);   // n03 = SaturateSignedDoublewordToSignedWord(buf[i : i+3] >> 7)
            int16x4_t n04 = vqshrn_n_s32(n02, 7);   // n04 = SaturateSignedDoublewordToSignedWord(buf[i+4 : i+7] >> 7)
            vst1_s16(&samples[i], n03);             // samples[i : i+3] = SaturateSignedDoublewordToSignedWord(buf[i : i+3] >> 7)
            vst1_s16(&samples[i+4], n04);           // samples[i+4 : i+7] = SaturateSignedDoublewordToSignedWord(buf[i+4 : i+7] >> 7)
        }
#else
        for (int i = 0; i < MIDI_UNIT; i++)
        {
            s32 d0 = buf[2 * i];
            s32 d1 = buf[2 * i + 1];
            for (int j = num_threads; j != 0; j--)
            {
                d0 += thread_info[j - 1].buf[2 * i];
                d1 += thread_info[j - 1].buf[2 * i + 1];
            }

            d0 >>= 7;
            if (d0 < -32768) d0 = -32768;
            else if (d0 > 32767) d0 = 32767;
            samples[2 * i] = d0;
            d1 >>= 7;
            if (d1 < -32768) d1 = -32768;
            else if (d1 > 32767) d1 = 32767;
            samples[2 * i + 1] = d1;
        }
#endif
    }
    else
#endif
    {
        memset(buf, 0, sizeof(s32) * 2 * MIDI_UNIT);
        gMan->Update(buf, MIDI_UNIT);
        gMan->Poll();

#ifdef __AVX2__
        for (int i = 0; i < MIDI_UNIT * 2; i += 16)
        {
            // buf is aligned
            __m256i mm1 = _mm256_load_si256((__m256i *) &buf[i]);       // mm1 = buf[i : i+7]
            __m256i mm2 = _mm256_load_si256((__m256i *) &buf[i + 8]);   // mm2 = buf[i+8 : i+15]
            mm1 = _mm256_srai_epi32(mm1, 7);                            // mm1 = buf[i : i + 7] >> 7
            mm2 = _mm256_srai_epi32(mm2, 7);                            // mm2 = buf[i+8 : i+15] >> 7
            mm1 = _mm256_packs_epi32(mm1, mm2);                         // mm1 = SaturateSignedDoublewordToSignedWord(buf[i : i+3] >> 7, buf[i+8 : i+11] >> 7, buf[i+4 : i+7] >> 7, buf[i+12 : i+15] >> 7)
            mm1 = _mm256_permute4x64_epi64(mm1, _MM_SHUFFLE(3,1,2,0));  // mm1 = SaturateSignedDoublewordToSignedWord(buf[i : i+15] >> 7)
            _mm256_storeu_si256((__m256i *) &samples[i], mm1);          // samples[i : i+15] = SaturateSignedDoublewordToSignedWord(buf[i : i+15] >> 7)
        }
#elif defined(__SSE2__)
        for (int i = 0; i < MIDI_UNIT * 2; i += 8)
        {
            // buf is aligned
            __m128i mm1 = _mm_load_si128((__m128i *) &buf[i]);      // mm1 = buf[i : i+3]
            __m128i mm2 = _mm_load_si128((__m128i *) &buf[i + 4]);  // mm2 = buf[i+4 : i+7]
            mm1 = _mm_srai_epi32(mm1, 7);                           // mm1 = buf[i : i + 3] >> 7
            mm2 = _mm_srai_epi32(mm2, 7);                           // mm2 = buf[i+4 : i+7] >> 7
            mm1 = _mm_packs_epi32(mm1, mm2);                        // mm1 = SaturateSignedDoublewordToSignedWord(buf[i : i+7] >> 7)
            _mm_storeu_si128((__m128i *) &samples[i], mm1);         // samples[i : i+7] = SaturateSignedDoublewordToSignedWord(buf[i : i+7] >> 7)
        }
#elif defined(__ARM_NEON__)
        for (int i = 0; i < MIDI_UNIT * 2; i += 8)
        {
            int32x4_t n01 = vld1q_s32(&buf[i]);     // n01 = buf[i : i+3]
            int32x4_t n02 = vld1q_s32(&buf[i + 4]); // n02 = buf[i+4 : i+7]
            int16x4_t n03 = vqshrn_n_s32(n01, 7);   // n03 = SaturateSignedDoublewordToSignedWord(buf[i : i+3] >> 7)
            int16x4_t n04 = vqshrn_n_s32(n02, 7);   // n04 = SaturateSignedDoublewordToSignedWord(buf[i+4 : i+7] >> 7)
            vst1_s16(&samples[i], n03);             // samples[i : i+3] = SaturateSignedDoublewordToSignedWord(buf[i : i+3] >> 7)
            vst1_s16(&samples[i+4], n04);           // samples[i+4 : i+7] = SaturateSignedDoublewordToSignedWord(buf[i+4 : i+7] >> 7)
        }
#else
        for (int i = 0; i < MIDI_UNIT; i++)
        {
            s32 d0 = buf[2 * i] >> 7;
            if (d0 < -32768) d0 = -32768;
            else if (d0 > 32767) d0 = 32767;
            samples[2 * i] = d0;
            d0 = buf[2 * i + 1] >> 7;
            if (d0 < -32768) d0 = -32768;
            else if (d0 > 32767) d0 = 32767;
            samples[2 * i + 1] = d0;
        }
#endif
    }
}

void FM_RenderSamplesFloat(float * RESTRICT samples0, float * RESTRICT samples1)
{
#if defined(ALIGN_BUF)
#if __cplusplus >= 201103L || _MSVC_LANG >= 201103L
    alignas(ALIGN_BUF)
#elif defined(__GNUC__)
    __attribute__ ((aligned (ALIGN_BUF)))
#elif defined(_MSC_VER)
    __declspec(align(ALIGN_BUF))
#else
#error unaligned variable
#endif
#endif
    s32 buf[MIDI_UNIT * 2];

#ifdef MULTI_THREAD
    if (ncore > 1)
    {
        int num_threads = ncore - 1;

        gMan->StartEnum();

        for (int i = num_threads; i != 0; i--)
        {
            fm_semaphore_signal(thread_info[i - 1].sem1);
        }

        memset(buf, 0, sizeof(s32) * 2 * MIDI_UNIT);
        while (!gMan->Update1(buf, MIDI_UNIT));

        for (int i = num_threads; i != 0; i--)
        {
            fm_semaphore_wait(thread_info[i - 1].sem2);
        }

        gMan->Poll();

#if defined(__SSE2__)
        __m128 mm0 = _mm_set1_ps(3e-7f);
        for (int i = 0; i < MIDI_UNIT; i += 4)
        {
            // buf is aligned
            __m128i mm1 = _mm_load_si128((__m128i *) &buf[2*i]);        // mm1 = buf[2*i : 2*i+3]
            __m128i mm2 = _mm_load_si128((__m128i *) &buf[2*i + 4]);    // mm2 = buf[2*i+4 : 2*i+7]
            for (int j = num_threads; j != 0; j--)
            {
                // thread_info[].buf is aligned
                __m128i mm1_2 = _mm_load_si128((__m128i *) &thread_info[j - 1].buf[2*i]);       // mm1_2 = thread_info[j - 1].buf[2*i : 2*i+3]
                __m128i mm2_2 = _mm_load_si128((__m128i *) &thread_info[j - 1].buf[2*i + 4]);   // mm2_2 = thread_info[j - 1].buf[2*i+4 : 2*i+7]
                mm1 = _mm_add_epi32(mm1, mm1_2);
                mm2 = _mm_add_epi32(mm2, mm2_2);
            }
            __m128i mm3 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(mm1), _mm_castsi128_ps(mm2), _MM_SHUFFLE(2,0,2,0)));
            __m128i mm4 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(mm1), _mm_castsi128_ps(mm2), _MM_SHUFFLE(3,1,3,1)));
            __m128 mm5 = _mm_cvtepi32_ps(mm3);
            __m128 mm6 = _mm_cvtepi32_ps(mm4);
            mm5 = _mm_mul_ps(mm5, mm0);
            mm6 = _mm_mul_ps(mm6, mm0);
            _mm_storeu_ps(&samples0[i], mm5);
            _mm_storeu_ps(&samples1[i], mm6);
        }
#elif defined(__ARM_NEON__)
        float32x4_t n00 = vdupq_n_f32(3e-7f);           // n00 = <3e-7f, ... , 3e-7f>
        for (int i = 0; i < MIDI_UNIT; i += 4)
        {
            int32x4_t n01 = vld1q_s32(&buf[2 * i]);     // n01 = buf[2*i : 2*i+3]
            int32x4_t n02 = vld1q_s32(&buf[2 * i + 4]); // n02 = buf[2*i+4 : 2*i+7]
            for (int j = num_threads; j != 0; j--)
            {
                // thread_info[].buf is aligned
                int32x4_t n01_2 = vld1q_s32(&thread_info[j - 1].buf[i]);        // n01_2 = thread_info[j - 1].buf[i : i+3]
                int32x4_t n02_2 = vld1q_s32(&thread_info[j - 1].buf[i + 4]);    // n02_2 = thread_info[j - 1].buf[i+4 : i+7]
                n01 = vaddq_s32(n01, n01_2);
                n02 = vaddq_s32(n02, n02_2);
            }
            float32x4_t n03 = vcvtq_f32_s32(n01);       // n03 = (float)buf[2*i : 2*i+3]
            float32x4_t n04 = vcvtq_f32_s32(n02);       // n04 = (float)buf[2*i+4 : 2*i+7]
            n03 = vmulq_f32(n03, n00);                  // n03 = 3e-7f * (float)buf[2*i : 2*i+3]
            n04 = vmulq_f32(n04, n00);                  // n04 = 3e-7f * (float)buf[2*i+4 : 2*i+7]
            float32x4x2_t n05 = vuzpq_f32(n03, n04);
            vst1q_f32(&samples0[i], n05.val[0]);
            vst1q_f32(&samples1[i], n05.val[1]);
        }
#else
        for (int i = 0; i < MIDI_UNIT; i++)
        {
            s32 d0 = buf[2 * i];
            s32 d1 = buf[2 * i + 1];
            for (int j = num_threads; j != 0; j--)
            {
                d0 += thread_info[j - 1].buf[2 * i];
                d1 += thread_info[j - 1].buf[2 * i + 1];
            }

            samples0[i] = 3e-7f * d0;
            samples1[i] = 3e-7f * d1;
        }
#endif
    }
    else
#endif
    {
        memset(buf, 0, sizeof(s32) * 2 * MIDI_UNIT);
        gMan->Update(buf, MIDI_UNIT);
        gMan->Poll();

#if defined(__ARM_NEON__)
        float32x4_t n00 = vdupq_n_f32(3e-7f);           // n00 = <3e-7f, ... , 3e-7f>
        for (int i = 0; i < MIDI_UNIT; i += 4)
        {
            int32x4_t n01 = vld1q_s32(&buf[2 * i]);     // n01 = buf[2*i : 2*i+3]
            int32x4_t n02 = vld1q_s32(&buf[2 * i + 4]); // n02 = buf[2*i+4 : 2*i+7]
            float32x4_t n03 = vcvtq_f32_s32(n01);       // n03 = (float)buf[2*i : 2*i+3]
            float32x4_t n04 = vcvtq_f32_s32(n02);       // n04 = (float)buf[2*i+4 : 2*i+7]
            n03 = vmulq_f32(n03, n00);                  // n03 = 3e-7f * (float)buf[2*i : 2*i+3]
            n04 = vmulq_f32(n04, n00);                  // n04 = 3e-7f * (float)buf[2*i+4 : 2*i+7]
            float32x4x2_t n05 = vuzpq_f32(n03, n04);
            vst1q_f32(&samples0[i], n05.val[0]);
            vst1q_f32(&samples1[i], n05.val[1]);
        }
#else
        for (int i = 0; i < MIDI_UNIT; i++)
        {
            samples0[i] = 3e-7f * buf[2 * i];
            samples1[i] = 3e-7f * buf[2 * i + 1];
        }
#endif
    }
}


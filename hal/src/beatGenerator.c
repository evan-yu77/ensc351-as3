#include "beatGenerator.h"
#include "audioMixer.h"
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

extern wavedata_t baseDrum;
extern wavedata_t hiHat;
extern wavedata_t snare;

static pthread_mutex_t beatMutex = PTHREAD_MUTEX_INITIALIZER;
static bool stopping = false;

static int currentBPM = 120;
static int currentMode = 1; 

typedef struct {
    bool playHiHat;
    bool playSnare;
    bool playBase;
} beat_step_t;

static const beat_step_t rockBeat[8] = {
    {true, false, true},  // Beat 1:   Hi-hat, Base
    {true, false, false}, // Beat 1.5: Hi-hat
    {true, true,  false}, // Beat 2:   Hi-hat, Snare
    {true, false, false}, // Beat 2.5: Hi-hat
    {true, false, true},  // Beat 3:   Hi-hat, Base
    {true, false, false}, // Beat 3.5: Hi-hat
    {true, true,  false}, // Beat 4:   Hi-hat, Snare
    {true, false, false}  // Beat 4.5: Hi-hat
};

static const beat_step_t customBeat[8] = {
    {true, true, false},
    {false, false, true},
    {true, false, true},
    {false, false, false},
    {true, true, false},
    {false, true, true},
    {true, true, true},
    {true, false, false}
};

void BeatGenerator_setMode(int newMode) {
    pthread_mutex_lock(&beatMutex);
    currentMode = newMode;
    pthread_mutex_unlock(&beatMutex);
}

void BeatGenerator_changeBPM(int bpmChange) {
    pthread_mutex_lock(&beatMutex);
    currentBPM += bpmChange;
    if (currentBPM > 300) currentBPM = 300;
    if (currentBPM < 40) currentBPM = 40;
    pthread_mutex_unlock(&beatMutex);
}

static void sleepForHalfBeat() {
    pthread_mutex_lock(&beatMutex);
    int bpm = currentBPM;
    pthread_mutex_unlock(&beatMutex);

    // Time For Half Beat [sec] = 60 / BPM / 2
    double seconds = 60.0 / bpm / 2.0; 
    long long nanoSeconds = (long long)(seconds * 1000000000.0);
    
    struct timespec reqDelay = {
        (time_t)(nanoSeconds / 1000000000), 
        (long)(nanoSeconds % 1000000000)
    };
    nanosleep(&reqDelay, (struct timespec *) NULL);
}


void* beatThread(void* arg) {
    int beatIndex = 0;
    
    while (!stopping) {
        pthread_mutex_lock(&beatMutex);
        int mode = currentMode;
        pthread_mutex_unlock(&beatMutex);

        if (mode == 1) { // 1 = Rock Beat
            if (rockBeat[beatIndex].playHiHat) {
                AudioMixer_queueSound(&hiHat);
            }
            if (rockBeat[beatIndex].playSnare) {
                AudioMixer_queueSound(&snare);
            }
            if (rockBeat[beatIndex].playBase)  {
                AudioMixer_queueSound(&baseDrum);
            }
        } else if (mode == 2) { // 2 = Custom Beat
            if (customBeat[beatIndex].playHiHat) {
                AudioMixer_queueSound(&hiHat);
            }
            if (customBeat[beatIndex].playSnare) {
                AudioMixer_queueSound(&snare);
            }
            if (customBeat[beatIndex].playBase)  {
                AudioMixer_queueSound(&baseDrum);
            }
        }

        // Advance to the next half-beat, or reset if the beat is turned off (mode 0)
        if (mode != 0) {
            beatIndex = (beatIndex + 1) % 8;
        } else {
            beatIndex = 0;
        }

        sleepForHalfBeat();
    }
    return NULL;
}

int BeatGenerator_getMode(void) {
    pthread_mutex_lock(&beatMutex);
    int mode = currentMode;
    pthread_mutex_unlock(&beatMutex);
    return mode;
}

int BeatGenerator_getBPM(void) {
    pthread_mutex_lock(&beatMutex);
    int bpm = currentBPM;
    pthread_mutex_unlock(&beatMutex);
    return bpm;
}
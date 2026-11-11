#include "textDisplay.h"
#include "beatGenerator.h"
#include "audioMixer.h"
#include "periodTimer.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

static pthread_t displayThreadId;
static bool stopping = false;

static void* displayThread(void* arg) {
    Period_statistics_t audioStats;
    Period_statistics_t accelStats;

    while (!stopping) {
        // Fetch current states
        int mode = BeatGenerator_getMode();
        int bpm = BeatGenerator_getBPM();
        int vol = AudioMixer_getVolume();

        // Fetch and clear statistics for the last second
        Period_getStatisticsAndClear(PERIOD_EVENT_AUDIO, &audioStats);
        Period_getStatisticsAndClear(PERIOD_EVENT_ACCEL, &accelStats);

        // Print the string format 
        printf("M%d %dbpm vol:%d  Audio[%.3f, %.3f] avg %.3f/%d  Accel[%.3f, %.3f] avg %.3f/%d\n",
            mode, bpm, vol,
            audioStats.minPeriodInMs, audioStats.maxPeriodInMs, audioStats.avgPeriodInMs, audioStats.numSamples,
            accelStats.minPeriodInMs, accelStats.maxPeriodInMs, accelStats.avgPeriodInMs, accelStats.numSamples);

        sleep(1); // Output once per second
    }
    return NULL;
}

void TextDisplay_init(void) {
    Period_init(); // Initialize the provided timer module first
    pthread_create(&displayThreadId, NULL, displayThread, NULL);
}

void TextDisplay_cleanup(void) {
    stopping = true;
    pthread_join(displayThreadId, NULL);
    Period_cleanup();
}
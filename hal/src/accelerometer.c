#include "accelerometer.h"
#include "audioMixer.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "periodTimer.h"

#define ACCEL_X_CH 1
#define ACCEL_Y_CH 2
#define ACCEL_Z_CH 3

#define SHAKE_THRESHOLD 300 
#define DEBOUNCE_TICKS 20 

extern wavedata_t baseDrum;
extern wavedata_t hiHat;
extern wavedata_t snare;


extern int readADC(int channel);

static bool stopping = false;
static pthread_t accelThreadId;

static void* accelerometerThread(void* arg) {
    int lastX = readADC(ACCEL_X_CH);
    int lastY = readADC(ACCEL_Y_CH);
    int lastZ = readADC(ACCEL_Z_CH);
    
    int debounceX = 0, debounceY = 0, debounceZ = 0;

    while (!stopping) {
        int x = readADC(ACCEL_X_CH);
        int y = readADC(ACCEL_Y_CH);
        int z = readADC(ACCEL_Z_CH);

        Period_markEvent(PERIOD_EVENT_ACCEL);

        // change in acceleration calculations
        int deltaX = abs(x - lastX);
        int deltaY = abs(y - lastY);
        int deltaZ = abs(z - lastZ);

        // hi-hat

        if (deltaX > SHAKE_THRESHOLD && debounceX == 0) {
            AudioMixer_queueSound(&hiHat);
            debounceX = DEBOUNCE_TICKS;
            printf("Air Drum: Hi-Hat! (Delta: %d)\n", deltaX);
        }

        // snare 

        if (deltaY > SHAKE_THRESHOLD && debounceY == 0) {
            AudioMixer_queueSound(&snare);
            debounceY = DEBOUNCE_TICKS;
            printf("Air Drum: Snare! (Delta: %d)\n", deltaY);
        }
        
        // base drum 

        if (deltaZ > SHAKE_THRESHOLD && debounceZ == 0) {
            AudioMixer_queueSound(&baseDrum);
            debounceZ = DEBOUNCE_TICKS;
            printf("Air Drum: Base! (Delta: %d)\n", deltaZ);
        }

        if (debounceX > 0) debounceX--;
        if (debounceY > 0) debounceY--;
        if (debounceZ > 0) debounceZ--;

        lastX = x;
        lastY = y;
        lastZ = z;

        // poll at 100 Hz (10ms)
        struct timespec reqDelay = {0, 10000000};
        nanosleep(&reqDelay, NULL);
    }
    return NULL;
}

void Accelerometer_init(void) {
    pthread_create(&accelThreadId, NULL, accelerometerThread, NULL);
}

void Accelerometer_cleanup(void) {
    stopping = true;
    pthread_join(accelThreadId, NULL);
}
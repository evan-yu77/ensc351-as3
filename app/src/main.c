#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include "audioMixer.h"
#include "beatGenerator.h"
#include "textDisplay.h"
#include "periodTimer.h"
#include "udp.h"
#include "joystick.h"
#include "encoder.h"
#include "accelerometer.h"
#define BASE_DRUM_FILE "beatbox-wav-files/04021f36f9dec72274b75b5f6af47d66/100051__menegass__gui-drum-bd-hard.wav"
#define HI_HAT_FILE "beatbox-wav-files/04021f36f9dec72274b75b5f6af47d66/100053__menegass__gui-drum-cc.wav"
#define SNARE_FILE "beatbox-wav-files/04021f36f9dec72274b75b5f6af47d66/100059__menegass__gui-drum-snare-soft.wav"

wavedata_t baseDrum;
wavedata_t hiHat;
wavedata_t snare;
volatile bool mainStopping = false;

int main() {
    printf("Initializing Audio Mixer...\n");
    Period_init();
    
    AudioMixer_init(); 
    
    AudioMixer_readWaveFileIntoMemory(BASE_DRUM_FILE, &baseDrum);
    AudioMixer_readWaveFileIntoMemory(HI_HAT_FILE, &hiHat);
    AudioMixer_readWaveFileIntoMemory(SNARE_FILE, &snare);

    pthread_t beatThreadId;
    pthread_create(&beatThreadId, NULL, beatThread, NULL);

    Joystick_init();
    encoder_init();
    Accelerometer_init();
    TextDisplay_init();

    if(!udp_init(&baseDrum, &hiHat, &snare)) {
        fprintf(stderr, "could not start udp\n");
        mainStopping = true;
    }
    
    while (!mainStopping && !udp_isStopRequested()) {
        sleep(1);
    }
    
    udp_cleanup();
    TextDisplay_cleanup();
    Accelerometer_cleanup();
    encoder_cleanup();
    Joystick_cleanup();

    BeatGenerator_stop();
    pthread_join(beatThreadId, NULL);

    AudioMixer_freeWaveFileData(&baseDrum);
    AudioMixer_freeWaveFileData(&hiHat);
    AudioMixer_freeWaveFileData(&snare);
    AudioMixer_cleanup();
    Period_cleanup();
    
    return 0;
}
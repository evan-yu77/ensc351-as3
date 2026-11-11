#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include "audioMixer.h"
#include "beatGenerator.h"
#define BASE_DRUM_FILE "beatbox-wav-files/04021f36f9dec72274b75b5f6af47d66/100051__menegass__gui-drum-bd-hard.wav"
#define HI_HAT_FILE "beatbox-wav-files/04021f36f9dec72274b75b5f6af47d66/100053__menegass__gui-drum-cc.wav"
#define SNARE_FILE "beatbox-wav-files/04021f36f9dec72274b75b5f6af47d66/100059__menegass__gui-drum-snare-soft.wav"

wavedata_t baseDrum;
wavedata_t hiHat;
wavedata_t snare;

int main() {
    printf("Initializing Audio Mixer...\n");
    
    AudioMixer_init(); 
    
    AudioMixer_readWaveFileIntoMemory(BASE_DRUM_FILE, &baseDrum);
    AudioMixer_readWaveFileIntoMemory(HI_HAT_FILE, &hiHat);
    AudioMixer_readWaveFileIntoMemory(SNARE_FILE, &snare);

    pthread_t beatThreadId;
    pthread_create(&beatThreadId, NULL, beatThread, NULL);

    while (true) {
        sleep(1);
    }
    
    AudioMixer_freeWaveFileData(&baseDrum);
    AudioMixer_freeWaveFileData(&hiHat);
    AudioMixer_freeWaveFileData(&snare);
    AudioMixer_cleanup();
    
    return 0;
}
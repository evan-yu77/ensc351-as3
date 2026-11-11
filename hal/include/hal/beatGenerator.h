#ifndef BEAT_GENERATOR_H
#define BEAT_GENERATOR_H

void* beatThread(void *arg);


void BeatGenerator_setMode(int newMode);
void BeatGenerator_changeBPM(int bpmChange);
int BeatGenerator_getMode(void);
int BeatGenerator_getBPM(void);

#endif

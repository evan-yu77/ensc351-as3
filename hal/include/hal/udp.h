#ifndef UDP_H
#define UDP_H

#include <stdbool.h>
#include "audioMixer.h"

#define UDP_PORT 12345

bool udp_init(wavedata_t *baseDrum, wavedata_t *hiHat, wavedata_t *snare);

bool udp_isStopRequested(void);

void udp_cleanup(void);

#endif
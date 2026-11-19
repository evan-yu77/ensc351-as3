#include "udp.h"
#include "audioMixer.h"
#include "beatGenerator.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_PACKET   256
#define MODE_MIN     0
#define MODE_MAX     2
#define VOLUME_MIN   0
#define VOLUME_MAX   100
#define TEMPO_MIN    40
#define TEMPO_MAX    300

static int socketFd = -1;
static pthread_t thread;
static bool threadRunning = false;
static volatile bool stopping = false;
static volatile bool stopRequested = false;

static wavedata_t *baseDrum, *hiHat, *snare;

static int clampInt(long v, int low, int high) {
    if (v < low) {
        return low;
    }
    if (v > high) {
        return high;
    }
    return (int)v;
}

static bool parseArg(const char *text, long *out) {
    char *end = NULL;
    errno = 0;
    long v = strtol(text, &end, 10);
    if (end == text || errno != 0) {
        return false;
    }
    while (*end == ' ' || *end == '\t') {
        end++;
    }
    if (*end != '\0') {
        return false;
    }
    *out = v;
    return true;
}

static void playSound(long id)
{
    switch (id) {
    case 0: AudioMixer_queueSound(baseDrum); break;
    case 1: AudioMixer_queueSound(hiHat);    break;
    case 2: AudioMixer_queueSound(snare);    break;
    }
}

static bool handleCommand(char *request, char *reply, size_t replySize)
{
    size_t len = strlen(request);
    while (len > 0 && (request[len - 1] == '\n' || request[len - 1] == '\r' || request[len - 1] == ' ')) {
        request[--len] = '\0';
    }
 
    char *arg = strchr(request, ' ');
    if (arg) {
        *arg = '\0';
        arg++;
        while (*arg == ' ') arg++;
    }
    const char *cmd = request;
 
    if (strcmp(cmd, "stop") == 0) {
        snprintf(reply, replySize, "ok stop");
        return true;
    }
 
    long value = 0;
    bool haveValue = arg != NULL && *arg != '\0' && strcmp(arg, "undefined") != 0 && parseArg(arg, &value);
 
    if (strcmp(cmd, "mode") == 0) {
        if (haveValue && value >= MODE_MIN && value <= MODE_MAX) {
            BeatGenerator_setMode((int)value);
        }
        snprintf(reply, replySize, "%d", BeatGenerator_getMode());
    } else if (strcmp(cmd, "volume") == 0) {
        if (haveValue) {
            AudioMixer_setVolume(clampInt(value, VOLUME_MIN, VOLUME_MAX));
        }
        snprintf(reply, replySize, "%d", AudioMixer_getVolume());
    } else if (strcmp(cmd, "tempo") == 0) {
        if (haveValue) {
            int target = clampInt(value, TEMPO_MIN, TEMPO_MAX);
            BeatGenerator_changeBPM(target - BeatGenerator_getBPM());
        }
        snprintf(reply, replySize, "%d", BeatGenerator_getBPM());
    } else if (strcmp(cmd, "play") == 0) {
        if (!haveValue || value < 0 || value > 2) {
            snprintf(reply, replySize, "error bad sound number");
        } else {
            playSound(value);
            snprintf(reply, replySize, "ok play %d", (int)value);
        }
    } else {
        snprintf(reply, replySize, "error unknown command");
    }
    return false;
}

static void *serverThread(void *unused) {
    (void)unused;
    char buffer[MAX_PACKET];
    char reply[MAX_PACKET];
    while (!stopping) {
        struct sockaddr_in sender;
        socklen_t senderLen = sizeof(sender);
        ssize_t n = recvfrom(socketFd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&sender, &senderLen);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            perror("UDP: recvfrom");
            break;
        }
        buffer[n] = '\0';

        bool isStop = handleCommand(buffer, reply, sizeof(reply));
        sendto(socketFd, reply, strlen(reply), 0, (struct sockaddr *)&sender, senderLen);

        if (isStop) {
            stopRequested = true;
            break;
        }
    }
    return NULL;
}

bool udp_init(wavedata_t *base, wavedata_t *hat, wavedata_t *snr) {
    if (threadRunning) {
        return false;
    }
    baseDrum = base;
    hiHat = hat;
    snare = snr;
    stopping = false;
    stopRequested = false;

    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd < 0) {
        perror("UDP: socket");
        return false;
    }
    int reuse = 1;
    setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct timeval timeout = { .tv_sec = 0, .tv_usec = 200000};
    setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(UDP_PORT);
    if (bind(socketFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("UDP: bind");
        close(socketFd);
        socketFd = -1;
        return false;
    }

    if (pthread_create(&thread, NULL, serverThread, NULL) != 0) {
        perror("UDP: pthread_create");
        close(socketFd);
        socketFd = -1;
        return false;
    }

    threadRunning = true;
    return true;
}

bool udp_isStopRequested(void) {
    return stopRequested;
}

void udp_cleanup(void) {
    if (!threadRunning) {
        return;
    }
    stopping = true;
    pthread_join(thread, NULL);
    threadRunning = false;
    close(socketFd);
    socketFd = -1;
}
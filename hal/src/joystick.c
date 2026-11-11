#include "joystick.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include "audioMixer.h"
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define SPI_DEV "/dev/spidev0.0"
#define JOYSTICK_Y_CHANNEL 0

static bool stopping = false;
static pthread_t joystickThreadId;
static int spi_fd = -1;
static pthread_mutex_t spiMutex = PTHREAD_MUTEX_INITIALIZER;

static void initSPI(void) {
    spi_fd = open(SPI_DEV, O_RDWR);
    if (spi_fd < 0) {
        perror("error: unable to open SPI device");
        exit(1);
    }

    int mode = 0;
    int bits = 8;
    int speed = 1000000; // 1 MHz

    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
}

int readADC(int channel) {
    if (channel < 0 || channel > 7) return -1;

    uint8_t tx[] = {
        0x01,                   
        0x80 | (channel << 4),  
        0x00                    
    };
    uint8_t rx[3] = {0};


    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .delay_usecs = 0,
        .speed_hz = 1000000,
        .bits_per_word = 8
    };



    pthread_mutex_lock(&spiMutex);

    int ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);

    pthread_mutex_unlock(&spiMutex);

    if (ret < 1) {
        perror("ERROR: SPI transaction failed");
        return -1;
    }

    int result = ((rx[1] & 0x0F) << 8) | rx[2];
    return result;
}

static int readJoystickY() {
    return readADC(JOYSTICK_Y_CHANNEL);
}

static void* joystickThread(void* arg) {
    int holdCounter = 0;
    int lastDirection = 0; // 0 = Neutral, 1 = Up, -1 = Down

    while (!stopping) {
        int yVal = readJoystickY();
        int currentDirection = 0;

        // Determine direction based on 12-bit ADC thresholds
        if (yVal > 3000) {
            currentDirection = 1;  // Up
        } else if (yVal < 1000) {
            currentDirection = -1; // Down
        }

        // 2. Process Auto-Repeat Logic
        if (currentDirection != 0) {
            if (currentDirection == lastDirection) {
                holdCounter++; // Stick is being held
            } else {
                holdCounter = 0; // Stick was just pushed
                lastDirection = currentDirection;
            }

            
            if (holdCounter == 0 || (holdCounter >= 40 && holdCounter % 10 == 0)) {
                int currentVol = AudioMixer_getVolume();
                
                if (currentDirection == 1) {
                    AudioMixer_setVolume(currentVol + 5);
                } else {
                    AudioMixer_setVolume(currentVol - 5);
                }
                printf("Volume changed to: %d\n", AudioMixer_getVolume());
            }
        } else {
            // Stick returned to neutral center
            holdCounter = 0;
            lastDirection = 0;
        }

        // Sleep for 10ms (100 Hz polling rate)
        struct timespec reqDelay = {0, 10000000}; 
        nanosleep(&reqDelay, NULL);
    }
    return NULL;
}

void Joystick_init(void) {
    initSPI();
    pthread_create(&joystickThreadId, NULL, joystickThread, NULL);
}

void Joystick_cleanup(void) {
    stopping = true;
    pthread_join(joystickThreadId, NULL);
    if (spi_fd >= 0) {
        close(spi_fd);
    }
}
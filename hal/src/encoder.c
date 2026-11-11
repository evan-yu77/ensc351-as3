#include "encoder.h"
#include "beatGenerator.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <gpiod.h>
#include <time.h>
#include <stdint.h>

#define MIN_FREQ 0
#define MAX_FREQ 500
#define INIT_FREQ 10
#define DEBOUNCE_US 3000             
#define MIN_CLICK_GAP_NS 5000000ULL
#define SW_DEBOUNCE_NS 200000000ULL

#define PIN_A_OFFSET 5  
#define PIN_B_OFFSET 6 
#define PIN_SW_OFFSET 7




static struct gpiod_chip *chip = NULL;
static struct gpiod_line_request *request = NULL; 


static pthread_t encoderThread;
static int currentMode = 1;
static volatile bool running = true;


static void* encoderThreadLoop(void* arg) {
    (void)arg;
    struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(16);
    if (!buffer) {
        perror("gpiod_edge_event_buffer_new");
        return NULL;
    }

    int levelB = (gpiod_line_request_get_value(request, 100000000));
    uint64_t lastRotNs = 0;
    uint64_t lastSwNs = 0;

    while (running) {
        int waitResult = gpiod_line_request_wait_edge_events(request, 100000000);
        if (waitResult < 0) {
            perror("gpiod_line_request_wait_edge_events");
            break;
        } else if (waitResult == 0) {
            continue;
        }

        int num_events = gpiod_line_request_read_edge_events(request, buffer, 16);
        if (num_events < 0) {
            perror("gpiod_line_request_read_edge_events");
            continue;
        }

        for (int i = 0; i < num_events; i++) {
            struct gpiod_edge_event *event = gpiod_edge_event_buffer_get_event(buffer, i);
            if (!event) continue;

            int offset = (int)gpiod_edge_event_get_line_offset(event);
            enum gpiod_edge_event_type type = gpiod_edge_event_get_event_type(event);
            uint64_t nowNs = gpiod_edge_event_get_timestamp_ns(event);

            if (offset == PIN_B_OFFSET) {
                levelB = (type == GPIOD_EDGE_EVENT_RISING_EDGE);
            } else if (offset == PIN_A_OFFSET && type == GPIOD_EDGE_EVENT_FALLING_EDGE) {
                uint64_t nowNs = gpiod_edge_event_get_timestamp_ns(event);
                if (nowNs - lastRotNs < MIN_CLICK_GAP_NS) {
                    continue;
                }
                lastRotNs = nowNs;

                if (levelB) {
                    BeatGenerator_changeBPM(5);
                } else {
                    BeatGenerator_changeBPM(-5);
                }
            }

            else if (offset == PIN_SW_OFFSET && type == GPIOD_EDGE_EVENT_FALLING_EDGE) {
                if (nowNs - lastSwNs < SW_DEBOUNCE_NS) {    
                    continue;
                }

                lastSwNs = nowNs;

                currentMode++;

                if (currentMode > 2) {
                    currentMode = 0;
                }

                BeatGenerator_setMode(currentMode);

                printf("Changed mode to: %d\n", currentMode);
            }
        }
    }
    gpiod_edge_event_buffer_free(buffer);
    return NULL;
}

void encoder_init(void) {

    chip = gpiod_chip_open("/dev/gpiochip3");
    if (!chip) {
        perror("Failed to open gpiochip3");
        exit(EXIT_FAILURE);
    }

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    struct gpiod_line_config *line_config = gpiod_line_config_new();

    if (!settings || !line_config) {
        perror("Failed to allocate line settings or config");
        exit(EXIT_FAILURE);
    }

    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
    gpiod_line_settings_set_debounce_period_us(settings, DEBOUNCE_US);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

    uint32_t offsets[] = {PIN_A_OFFSET, PIN_B_OFFSET, PIN_SW_OFFSET};
    for (int i = 0; i < 3; i++) {
        gpiod_line_config_add_line_settings(line_config, &offsets[i], 1, settings);
    }



    request = gpiod_chip_request_lines(chip, NULL, line_config);

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_config);

    if (!request) {
        perror("Failed to request lines");
        exit(EXIT_FAILURE);
    }

    running = true;
    pthread_create(&encoderThread, NULL, encoderThreadLoop, NULL);
}

void encoder_cleanup(void) {
    running = false;
    pthread_join(encoderThread, NULL);

    if (request) {
        gpiod_line_request_release(request);
    }

    if (chip) {
        gpiod_chip_close(chip);
    }
}


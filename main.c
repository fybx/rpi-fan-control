/*
 *      Ferit Yiğit BALABAN,    fybalaban@fybx.dev
 *      rpi-fan-control         2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pigpio.h>

#define TEMP_PATH "/sys/class/thermal/thermal_zone0/temp"
#define FAN_PIN 17

#define HIGH 1
#define LOW 0

int main(int argc, char** argv) {
    int fd;
    char buffer[6];
    int fanRunning;
    double temperature;
    int threshold;
    int variance;

    if (argc != 3) {
        printf("Expected threshold temperature and variance\n");
        return -1;
    }

    threshold = atoi(*(argv + 1));
    variance  = atoi(*(argv + 2));

    if (!threshold) {
        printf("Expected integer for threshold temperature: %s\n", *(argv + 1));
        return -2;
    } else if (!variance) {
        printf("Expected integer for variance: %s\n", *(argv + 2));
        return -3;
    }

    if (gpioInitialise() < 0) {
        printf("Failed to initialize GPIO.\n");
        return 1;
    }

    gpioSetMode(FAN_PIN, PI_INPUT);
    fanRunning = gpioRead(FAN_PIN);
    gpioSetMode(FAN_PIN, PI_OUTPUT);

    fd = open(TEMP_PATH, O_RDONLY);
    if (fd < 0) {
        printf("Error opening temperature file\n");
        return 2;
    }

    if (read(fd, buffer, sizeof(buffer)) < 0) {
        printf("Error reading temperature\n");
        close(fd);
        return 3;
    }

    close(fd);
    temperature = atof(buffer) / 1000.0;
    printf("CPU Temperature: %.2f°C\n", temperature);

    if (!fanRunning && temperature >= threshold + variance) {
       gpioWrite(FAN_PIN, HIGH);
       printf("Setting pin GPIO17 to HIGH\n");
    } else if (temperature <= threshold - variance) {
        gpioWrite(FAN_PIN, LOW);
        printf("Setting pin GPIO17 to LOW\n");
    } else {
        printf("Pin GPIO17 is already HIGH\n");
    }

    gpioTerminate();
    return 0;
}

/*
 *      Ferit Yiğit BALABAN,    fybalaban@fybx.dev
 *      rpi-fan-control         2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pigpio.h>
#include <time.h>

#define TEMP_PATH "/sys/class/thermal/thermal_zone0/temp"
#define FAN_PIN 17
#define LOG 1
#define LOG_FILE "/var/log/fan_control.log"

#define HIGH 1
#define LOW 0

int logMessage(const char *, ...);

int getFanStatus();

int writeFanStatus(int);

int main(int argc, char **argv) {
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

int logMessage(const char *format, ...) {
    if (!LOG) {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"
        return 0;
#pragma clang diagnostic pop
    }

    FILE *logFile = fopen(LOG_FILE, "a");
    if (!logFile) {
        printf("Error opening log file");
        return 1;
    }

    time_t currentTime;
    time(&currentTime);
    struct tm *timeInfo = localtime(&currentTime);

    fprintf(logFile, "[%04d-%02d-%02d %02d:%02d:%02d] ", timeInfo->tm_year + 1900, timeInfo->tm_mon + 1,
            timeInfo->tm_mday, timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);

    va_list args;
    va_start(args, format);
    vfprintf(logFile, format, args);
    va_end(args);

    fclose(logFile);
    return 0;
}

int getFanStatus() {
    int status;
    FILE *file = fopen(STATUS, "r");

    if (!file) {
        logMessage("Error opening status file\n");
        return -1;
    }

    if (fscanf(file, "%d", &status) != 1) {
        logMessage("Error reading status file\n");
        fclose(file);
        return -2;
    }

    fclose(file);
    return status;
}

int writeFanStatus(int status) {
    if (status != LOW && status != HIGH) {
        logMessage("Invalid status: %d\n", status);
        return 1;
    }

    FILE *f = fopen(STATUS, "w");
    if (!f) {
        logMessage("Error opening status file.\n");
        return 2;
    }

    fprintf(f, "%d", status);
    fclose(f);
    return 0;
}

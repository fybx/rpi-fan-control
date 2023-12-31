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
#define STATUS "/var/log/fanstatus"

#define HIGH 1
#define LOW 0

int logMessage(const char *, ...);

int getFanStatus();

int writeFanStatus(int);

int getCoreTemperature();

int main(int argc, char **argv) {
    int fanRunning;
    double temperature;
    int threshold;
    int variance;
    int function;

    if (argc == 2) {
        if (!strcmp(*(argv + 1), "on")) {
            logMessage("Unexpected token: %s\n", *(argv + 1));
            return 1;
        } else
            function = 1;
    } else if (argc == 3) {
        threshold = atoi(*(argv + 1));
        variance = atoi(*(argv + 2));

        if (!threshold) {
            logMessage("Expected integer for threshold temperature: %s\n", *(argv + 1));
            return 2;
        } else if (!variance) {
            logMessage("Expected integer for variance: %s\n", *(argv + 2));
            return 3;
        } else {
            function = 0;
        }
    } else {
        logMessage("Expected threshold temperature and variance\n");
        return 4;
    }

    if (gpioInitialise() < 0) {
        logMessage("Failed to initialize GPIO.\n");
        return 5;
    }

    fanRunning = getFanStatus();
    logMessage("Fan status: %d\n", fanRunning);
    gpioSetMode(FAN_PIN, PI_OUTPUT);

    temperature = getCoreTemperature();
    logMessage("CPU Temperature: %.2f°C\n", temperature);

    if (!fanRunning && temperature >= threshold + variance) {
        fanRunning = HIGH;
        logMessage("Setting pin GPIO17 to HIGH\n");
    } else {
        if (temperature <= threshold - variance) {
            fanRunning = LOW;
            logMessage("Setting pin GPIO17 to LOW\n");
        } else
            logMessage("Pin GPIO17 is already HIGH\n");
    }
    gpioWrite(FAN_PIN, fanRunning);

    writeFanStatus(fanRunning);
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

int getCoreTemperature() {
    int fd;
    char buffer[6];

    fd = open(TEMP_PATH, O_RDONLY);
    if (fd < 0) {
        logMessage("Error opening temperature file\n");
        return -1;
    }

    if (read(fd, buffer, sizeof(buffer)) < 0) {
        logMessage("Error reading temperature\n");
        close(fd);
        return -2;
    }

    close(fd);
    return (atof(buffer) / 1000.0);
}

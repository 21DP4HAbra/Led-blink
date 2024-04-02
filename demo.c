#include "ch347_lib.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h> // for sleep()

#define GPIO_PIN 1

int main() {
    int fd;
    bool ret;
    const char *devicePath = "/dev/ch347device"; // You need to adjust this path as per your setup

    // Open the CH347 device
    fd = CH347OpenDevice(devicePath);
    if (fd < 0) {
        fprintf(stderr, "Failed to open the CH347 device.\n");
        return -1;
    }

    uint8_t iEnable = (1 << GPIO_PIN); // Enable GPIO function for GPIO 1
    uint8_t iSetDirOut = (1 << GPIO_PIN); // Set GPIO 1 as output
    uint8_t iSetDataOut = 0x00; // Start with LED off

    // Configure GPIO 1 as output
    ret = CH347GPIO_Set(fd, iEnable, iSetDirOut, iSetDataOut);
    if (!ret) {
        fprintf(stderr, "Failed to initialize GPIO 1 as output.\n");
        CH347CloseDevice(fd);
        return -1;
    }

    // Loop to flash the LED on GPIO 1
    while (1) {
        
        // Toggle the LED state
        iSetDataOut ^= (1 << GPIO_PIN); // Toggle the bit for GPIO 1
        ret = CH347GPIO_Set(fd, iEnable, iSetDirOut, iSetDataOut);
        if (!ret) {
            fprintf(stderr, "Failed to set GPIO 1 state.\n");
            break;
        }

        sleep(1); // Wait for 1 second
    }

    // Clean up and close the device
    CH347CloseDevice(fd);

    return 0;
}

#include "CH347DLL_EN.h"
#include <Windows.h> // For Sleep function and GetAsyncKeyState

int main() {
    // Idk how to get deviceID
    ULONG deviceIndex = 0;
    UCHAR gpioDirection;
    UCHAR gpioData;
    
    // Open the device
    HANDLE handle = CH347OpenDevice(deviceIndex);
    if (handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to open device\n");
        return 1;
    }

    // Get the state of the GPIO
    CH347GPIO_Get(deviceIndex, gpioDirection, gpioData);

    // Ensure GPIO 1 is set as an output
    gpioDirection |= 0x01; // Set GPIO 1 as output

    CH347GPIO_Set(deviceIndex, 0xFF, gpioDirection, gpioData);

    // Blink loop while space bar pressed
    while (GetAsyncKeyState(VK_SPACE)) {
        // Set GPIO 1 high
        CH347GPIO_Set(deviceIndex, 0x01, gpioDirection, 0x02);
        Sleep(1000); // 1 second delay

        if (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) break; // Check again if space bar is pressed

        // Set GPIO 1 low
        CH347GPIO_Set(deviceIndex, 0xFF, gpioDirection, 0x00); // GPIO 1 low
        Sleep(1000); // 1 second delay
    }

    // Close the device
    CH347CloseDevice(deviceIndex);

    return 0;
}

#include "ch347_lib.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

struct ch34x {
	int fd;
	char version[100];
	CHIP_TYPE chiptype;
	uint32_t dev_id;
};

static struct ch34x ch347device;

int FlashGPIO() {
    bool ret;
	int i, j;
	int gpiocount = 8;
	uint8_t iEnable = 0xff;
	uint8_t iSetDirOut = 0xff;
	uint8_t iSetDataOut = 0x00;

    CH347GPIO_Set(ch347device.fd, iEnable, iSetDirOut, iSetDataOut);

    // Set GPIO 1 high
    CH347GPIO_Set(ch347device.fd, iEnable, iSetDirOut, iSetDataOut);
    Sleep(1000); // 1 second delay

    // Set GPIO 1 low
    CH347GPIO_Set(ch347device.fd, iEnable, iSetDirOut, iSetDataOut); // GPIO 1 low
    Sleep(1000); // 1 second delay
    
    return 0;
}

int main (int argc, char *argv[]) {
    // open device
	ch347device.fd = CH347OpenDevice(argv[1]);
	if (ch347device.fd < 0) {
		printf("CH347OpenDevice() false.\n");
		return -1;
	};

    while(1){
        FlashGPIO();
    };
};
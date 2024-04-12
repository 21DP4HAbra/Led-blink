// Creator: Justs Abrams

#include "ch347_lib.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#define _GNU_SOURCE

// Define intervals for sleep so its easier on the eyes
#define HALF_SECOND 500000
#define ONE_AND_HALF_SECONDS 1500000

struct ch34x {
	int fd;
	char version[100];
	CHIP_TYPE chiptype;
	uint32_t dev_id;
};

static struct ch34x device;

// Function for outputing current GPIO status
static void GPIO_read() {

  uint8_t iDir;
  uint8_t iData;

  CH347GPIO_Get(device.fd, &iDir, &iData);
  //Print iDir pointer
  printf("GPIO direction bit [GET]: %p, ", iDir);
  //Print iData as hex
  printf("iData value is: %X\n", iData);
}

//Function for blinking GPIO #1 ready to update
static void GPIO_blink_ready_to_update() {
	
	int i;
	uint8_t iDir;
	uint8_t iData;
	uint8_t iEnable;
	uint8_t iSetDirOut;
	uint8_t iSetDataOut;

	//Get GPIO status
  	CH347GPIO_Get(device.fd, &iDir, &iData);
  	iSetDataOut = iData;


	for (i = 0; i < 10; i++) {
		iSetDataOut = iSetDataOut ^ 0x02;
		CH347GPIO_Set(device.fd, 0x02, 0x02, iSetDataOut);

		usleep(HALF_SECOND);
	}
}

//Function for blinking GPIO #1 update finished
static void GPIO_blink_finished() {
	
	int i;
	uint8_t iDir;
	uint8_t iData;
	uint8_t iEnable;
	uint8_t iSetDirOut;
	uint8_t iSetDataOut;

	//Get GPIO status
  	CH347GPIO_Get(device.fd, &iDir, &iData);
  	iSetDataOut = iData;


	for (i = 0; i < 10; i++) {
		iSetDataOut = iSetDataOut ^ 0x02;
		CH347GPIO_Set(device.fd, 0x02, 0x02, iSetDataOut);

		usleep(ONE_AND_HALF_SECONDS);
	}
}



int main (int argc, char *argv[]) {

	bool update_ready;
	int input;
    int toggleState = 0;

	GPIO_read();
    // open GPIO device
	printf("GPIO Device is opening...");
	device.fd = CH347OpenDevice(argv[1]);
	if (device.fd < 0) {
		printf("GPIO failed to open false.\n");
		return -1;
	};
	printf("GPIO device %s opened successfully, fd: %d\n", argv[1], device.fd);

	// Mock the different modes of led blinking
	system("clear");
    printf("Press 0 to toggle \"Ready to update \" or press 1 to toggle \"Update finished\" blinking modes\n");

    while (1) {
        printf("Enter your choice: ");
        scanf("%d", &input); // Read an integer input from the user

        if (input == 1) {
			system("clear");
            toggleState = !toggleState; // Toggle the state
            printf("State is now: %s\n", toggleState ? "Update finished" : "Ready to update\n");
            GPIO_blink_ready_to_update();
            break;
        } else if (input == 0) {
			system("clear");
            printf("State is now: %s\n", toggleState ? "Update finished" : "Ready to update\n");
            GPIO_blink_finished();
            break;
        } else {
			system("clear");
            printf("Invalid input. Please enter '1' to toggle or '0' to quit.\n");
        }
    }
	system("clear");
    printf("Exiting program, wait for device to close.\n");

		/* close the device */
	if (CH347CloseDevice(device.fd)) {
		system("clear");
		printf("Close device succeed.\n");
	}
    return 0;
};
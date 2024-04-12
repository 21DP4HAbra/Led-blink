#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "ch347_lib.h"

#define SOCKET_NAME "/run/ddos-aa-control.sock"
#define BUFFER_SIZE 32

#define _GNU_SOURCE // Required for accept4() function TODO: not working as
                    // described

#define GPIO_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define GPIO_TO_BINARY(gpio_returned_value)                                    \
  ((gpio_returned_value) & 0x80 ? '1' : '0'),                                  \
      ((gpio_returned_value) & 0x40 ? '1' : '0'),                              \
      ((gpio_returned_value) & 0x20 ? '1' : '0'),                              \
      ((gpio_returned_value) & 0x10 ? '1' : '0'),                              \
      ((gpio_returned_value) & 0x08 ? '1' : '0'),                              \
      ((gpio_returned_value) & 0x04 ? '1' : '0'),                              \
      ((gpio_returned_value) & 0x02 ? '1' : '0'),                              \
      ((gpio_returned_value) & 0x01 ? '1' : '0')

static volatile bool force_quit;

struct ch34x {
  int fd;
  char version[100];
  CHIP_TYPE chiptype;
  uint32_t dev_id;
};

static struct ch34x ch347device;

/* Handle SIGINT|SIGTERM event */
static void signal_handler(int signum) {

  if (signum == SIGINT || signum == SIGTERM) {
    printf("\n\nSignal %d received, preparing to exit...\n", signum);
    force_quit = true;
  }
}

void *connection_handler(void *socket_desc) {
  // Get the socket descriptor
  int sock = *(int *)socket_desc;
  int read_size;
  char *message, client_message[2000];

  // Send some messages to the client
  printf("Socket listing phase, data_socket value is: %d\n", sock);
  write(sock, "LIC1PRT0RSV0RSV0SIDARST0SWU0DMP0\n", 32);

  // Receive a message from client
  while ((read_size = recv(sock, client_message, 2000, 0)) > 0) {
    // end of string marker
    client_message[read_size] = '\0';

    // Send the message back to client
    write(sock, client_message, strlen(client_message));

    // clear the message buffer
    memset(client_message, 0, 2000);
  }

  if (read_size == 0) {
    printf("Client disconnected\n");
    fflush(stdout);
  } else if (read_size == -1) {
    printf("recv failed\n");
  }
  // Client closed socket so clean up
  close(sock);
  return 0;
}

void *connection_listener(void *socket_desc) {
  int sock = *(int *)socket_desc;
  int data_socket;
  pthread_t thread_id;
  printf("Socket is waiting for incoming connections...\n");

  while ((data_socket = accept(sock, NULL, NULL))) {
    printf("Connection accepted\n");

    if (pthread_create(&thread_id, NULL, connection_handler,
                       (void *)&data_socket) < 0) {
      printf("could not create thread\n");
    }

    // Now join the thread , so that we dont terminate before the thread
    // pthread_join( thread_id , NULL);
    printf("Handler assigned\n");
  }
}

/* Read current GPIO signal levels */
static void ch34x_gpio_read() {

  uint8_t iDir;
  uint8_t iData;

  /* Get current GPIO signal levels */
  CH347GPIO_Get(ch347device.fd, &iDir, &iData);
  printf("Current GPIO direction bit [GET]: %p, ", iDir);
  printf("iData [GET] value is: %X\n", iData);

  usleep(200 * 4000);
}

/* Check PROTECT mode toggle switch */
static bool protect_mode_get() {

  uint8_t iDir;
  uint8_t iData;
  bool protect_mode = false;

  usleep(200 * 4000);
  CH347GPIO_Get(ch347device.fd, &iDir, &iData);
  protect_mode = ((iData & 0x01) >> 0); // Fetch GPIO#1 signal level

  printf("Function 'protect_mode_get', iData [GET] value is: %X, ", iData);
  printf(GPIO_TO_BINARY_PATTERN,
         GPIO_TO_BINARY(iData)); // Print all GPIO channels states in binary
  printf(", PROTECT mode switch position: %s\n",
         protect_mode ? "true" : "false");

  return protect_mode;
}

/* Set operational LED */
static void ch34x_gpio_operational() {

  bool ret;

  ret = CH347GPIO_Set(ch347device.fd, 0x20, 0x20,
                      0x20); // Set GPIO#5 OPERATIONAL LED to ON
  if (!ret) {
    printf("Failed to set GPIO#5 'operational' LED!\n");
  }
}

/* Unset operational LED */
static void ch34x_gpio_non_operational() {

  bool ret;

  ret = CH347GPIO_Set(ch347device.fd, 0x20, 0x20,
                      0x00); // Set GPIO#5 OPERATIONAL LED to OFF
  if (!ret) {
    printf("Failed to set GPIO#5 'non-operational' LED!\n");
  }
}

/* Check for USB media presence and existence of valid SW update */
static bool sw_update_is_ready(char *sw_update_ready_filename) {

  FILE *sw_update_file;

  sw_update_file = fopen(sw_update_ready_filename, "r");

  if (sw_update_file == NULL)
    return false;
  fclose(sw_update_file);
  return true;
}

/* Toggle GPIO#1 signal level (LED blinking) during SW UPDATE process*/
static void ch34x_gpio_blink() {

  // bool ret;
  int i;
  uint8_t iDir;
  uint8_t iData;
  uint8_t iEnable = 0xFF;
  uint8_t iSetDirOut = 0xFF;
  uint8_t iSetDataOut = 0xED;

  /* Get current GPIO signal levels */
  CH347GPIO_Get(ch347device.fd, &iDir, &iData);
  iSetDataOut = iData;

  for (i = 0; i < 2; i++) {
    iSetDataOut = iSetDataOut ^ 0x02;
    CH347GPIO_Set(ch347device.fd, 0x02, 0x02, iSetDataOut);
    // printf("iSetDataOut [SET] value is: %X\n\n", iSetDataOut);

    usleep(200 * 500);
  }
}

static void ch34x_gpio_blink_off() {

  CH347GPIO_Set(ch347device.fd, 0x02, 0x02, 0x00);
}

int main(int argc, char *argv[]) {

  struct sockaddr_un ddos_aa_control;
#if 0
    struct sockaddr_un {
        sa_family_t sun_family;               // AF_UNIX type
        char        sun_path[108];            // pathname
    };
#endif

  int i;
  int ret = 0;
  int connection_socket;
  char buffer[BUFFER_SIZE];
  bool ddos_aa_is_running = false;
  bool protect_mode = false;
  char *sw_update_ready_filename = "./USB";
  bool update_is_ready = false;

  if (argc != 2) {
    printf("Usage: %s [DEVICE]\n", argv[0]);
    return -1;
  }

  /* Open GPIO device */
  ch347device.fd = CH347OpenDevice(argv[1]);
  if (ch347device.fd < 0) {
    printf("GPIO device open failed!\n");
    return -1;
  }
  printf("GPIO device %s open succeed, fd: %d\n", argv[1], ch347device.fd);

  /* Create UNIX STREAM socket for communication with 'ddos-aa' application */
  unlink(SOCKET_NAME); // Remove a remined socket in case of previous
                       // inadvertenly exited program

  connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  // connection_socket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
  // //TODO: maybe SOCK_DGRAM ?
  if (connection_socket == -1) {
    printf("UNIX STREAM socket creation failed! Exiting...\n");
    return -1;
  }

  memset(&ddos_aa_control, 0, sizeof(struct sockaddr_un));
  ddos_aa_control.sun_family = AF_UNIX;
  strncpy(ddos_aa_control.sun_path, SOCKET_NAME,
          sizeof(ddos_aa_control.sun_path) - 1);

  ret = bind(connection_socket, (const struct sockaddr *)&ddos_aa_control,
             sizeof(struct sockaddr_un));
  printf("Socket binding phase, ret value is: %d\n", ret);
  if (ret == -1) {
    printf("UNIX STREAM socket binding failed! Exiting...\n");
    return -1;
  }

  ret = listen(connection_socket,
               5); // Depth of connection requests queue defined as argument
  printf("Socket listen phase, ret value is: %d\n", ret);
  if (ret == -1) {
    printf("UNIX STREAM socket listen failed! Exiting...\n");
    return -1;
  }

  printf("UNIX STREAM socket has created succesfully, status code: %d\n",
         connection_socket);

  /* Set the initial (factory) GPIO card state */
  CH347GPIO_Set(ch347device.fd, 0xFF, 0xFF, 0xED);
  printf("GPIO device set to its initial (factory) state\n\n");
  usleep(200 * 3000); // Keep initial LEDs status during initialization

  /* Welcome OPERATIONAL LED fast blinking */
  for (i = 6; i > 0; i--) {
    CH347GPIO_Set(ch347device.fd, 0x20, 0x20, 0x00);
    usleep(200 * ((i * 150) + 400));
    CH347GPIO_Set(ch347device.fd, 0x20, 0x20, 0x20);
    usleep(100 * ((i * 350) + 200));
  }
 CH347GPIO_Set(ch347device.fd, 0x22, 0x22,
                0x00); // Set OPERATIONAL and SW UPDATE LEDs to OFF
  printf("Turning OPERATIONAL and SW UPDATE LEDs initially to OFF...\n");
  printf("iEnable [SET] value is: %X, iSetDirOut [SET] value is: %X, "
         "iSetDataOut [SET] value is: %X\n\n",
         0x22, 0x22, 0x00);
  usleep(200 * 1000);

  force_quit = false;
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  while (!force_quit) {
    /* Listening a UNIX STREAM socket for 'ddos-aa' connection requests */
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, connection_listener,
                       (void *)&connection_socket) < 0) {
      perror("could not create thread");
      return 1;
    }
    // data_socket = accept4(connection_socket, NULL, 0, SOCK_NONBLOCK); //TODO:
    // accept4 not compiling
    // printf("Socket listing phase, data_socket value is: %d\n", data_socket);
    // write(data_socket, "LIC1PRT0RSV0RSV0SIDARST0SWU0DMP0\n", 32);

    // ch34x_gpio_read();

    /* Check for 'ddos-aa' process running state & PROTECT mode [ON] postion */
    protect_mode = protect_mode_get();
    if (ddos_aa_is_running != true &&
        protect_mode == true) { // TODO: complete code
      ch34x_gpio_operational();
    } else {
      ch34x_gpio_non_operational();
    }

    /* Check USB file presence and existence of valid SW update */
    update_is_ready = sw_update_is_ready(sw_update_ready_filename);

    if (update_is_ready) {
      ch34x_gpio_blink();
    } else {
      ch34x_gpio_blink_off();
    }
 /* Close UNIX STREAM socket */
  close(connection_socket);
  unlink(SOCKET_NAME);
  printf("\nUNIX STREAM socket closed\n");

  /* Close GPIO device */
  if (CH347CloseDevice(ch347device.fd)) {
    printf("GPIO device closed\n");
  }

  return 0;
}



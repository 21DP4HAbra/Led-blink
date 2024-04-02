static void ch34x_demo_gpio_operate()
{
    bool ret;
    int i, j;
    int gpiocount = 8;
    uint8_t iEnable = 0xff;
    uint8_t iSetDirOut = 0xff;
    uint8_t iSetDataOut = 0x00;

    /* analog leds here */
    CH347GPIO_Set(ch347device.fd, iEnable, iSetDirOut, iSetDataOut);

    printf("\n********** GPIO Output Start **********\n");
    for (i = 0; i < gpiocount; i++) {
        iSetDataOut = 1 << i;
        ret = CH347GPIO_Set(ch347device.fd, iEnable, iSetDirOut, iSetDataOut);
        if (ret == false) {
            printf("CH347GPIO_Set error.\n");
            return;
        }
        printf("\n");
        for (j = 0; j < gpiocount; j++) {
            if (j == i)
                printf("H");
            else
                printf("L");
        }
        printf("\n");
    }
    usleep(200 * 1000);
    iSetDataOut = 0x00;
    CH347GPIO_Set(ch347device.fd, iEnable, iSetDirOut, iSetDataOut);
    printf("\n********** GPIO Output End **********\n\n");
}
https://github.com/WCHSoftGroup/ch341par_linux/tree/main/demo/ch347
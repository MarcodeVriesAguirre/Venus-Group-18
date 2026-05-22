#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <libpynq.h>

#define UART_CHANNEL UART0

void send_message(const int uart, const char *payload)
{
    uint32_t len = strlen(payload);

    uint8_t header[4];

    header[0]=(len>>24)&0xFF;
    header[1]=(len>>16)&0xFF;
    header[2]=(len>>8)&0xFF;
    header[3]=len&0xFF;

    for(int i=0;i<4;i++)
    {
        uart_send(uart,header[i]);
    }

    for(uint32_t i=0;i<len;i++)
    {
        uart_send(uart,payload[i]);
    }
}

int main()
{
    pynq_init();

    /*
     connect UART to pins
    */

  switchbox_set_pin(IO_AR0, SWB_UART0_RX);
    switchbox_set_pin(IO_AR1, SWB_UART0_TX);

    uart_init(UART_CHANNEL);

    uart_reset_fifos(UART_CHANNEL);

    const char *payload=
    "ROBOT=A;"
    "TYPE=TEST;"
    "MSG=HELLO_FROM_PYNQ";

    while(1)
    {
        printf("Sending: %s\n",payload);

        send_message(UART_CHANNEL,payload);

        sleep(2);
    }

    pynq_destroy();

    return 0;
}
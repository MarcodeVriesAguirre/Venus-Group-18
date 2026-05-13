#include <libpynq.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

void send_uart_msg(const char format, ...) {
    char payload[256];
    va_list args;

    va_start(args, format);
    vsnprintf(payload, sizeof(payload), format, args);
    va_end(args);

    uint32_t len = (uint32_t)strlen(payload) + 1;

    if(gpio_get_level(IO_AR3) == GPIO_LEVEL_LOW) {
        printf("Waiting for ESP32 to be ready on pin AR3...\n");
        // Loop until AR3 goes HIGH
        while(gpio_get_level(IO_AR3) == GPIO_LEVEL_LOW) {
            sleep_msec(10); 
        }
    }

    uint8_tlen_bytes = (uint8_t)&len;
    for(int i = 0; i < 4; i++) {
        uart_send(UART0, len_bytes[i]);
    }

    for(uint32_t i = 0; i < len; i++) {
        uart_send(UART0, payload[i]);
    }
}

double get_temperature() { return 24.5; }
int get_distance() { return 150; }
const char get_color() { return "RED"; }

int main(void) {
    pynq_init();
    adc_init();
    buttons_init(); 
    switchbox_init(); 

    gpio_set_direction(IO_AR3, GPIO_DIR_INPUT);

    switchbox_set_pin(IO_AR0, SWB_UART0_RX);
    switchbox_set_pin(IO_AR1, SWB_UART0_TX);

    uart_init(UART0);
    uart_reset_fifos(UART0); // Clear out any old junk data

    printf("Robot 49 is starting... Press Button 0 to stop.\n");

    while(!get_button_state(BUTTON0)) {
        double temp = get_temperature();
        int dist    = get_distance();
        const char* color = get_color();

        // Send the JSON string to the ESP32
        send_uart_msg("{"id":"49", "temp": %.2f, "dist": %d, "color": "%s"}", temp, dist, color);

        printf("Sent data: Temp %.2f, Dist %d, Color %s\n", temp, dist, color);
        sleep_msec(500);
    }

    printf("\nShutting down...\n");
    uart_destroy(UART0);
    switchbox_destroy();
    adc_destroy();
    buttons_destroy();
    pynq_destroy();
    return 0;
}
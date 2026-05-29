#include <libpynq.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PIN_D0 IO_AR9

int main(void) {
  	pynq_init();

	switchbox_set_pin(PIN_D0, SWB_GPIO);
	gpio_set_direction(PIN_D0, GPIO_DIR_INPUT);

    setbuf(stdout, NULL);
    printf("Reading\n");

    while(1){
        int digital = gpio_get_level(PIN_D0);

        printf("%d\n", digital);
        sleep(1);
    }
    
	pynq_destroy();
	return EXIT_SUCCESS;
}

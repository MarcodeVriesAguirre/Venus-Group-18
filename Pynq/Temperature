#include <stdio.h>
#include <libpynq.h>
#include <math.h>


#define BETA 4050.0
#define V_REF 3.3
#define R2 1000.0       // fixed resistor = 1k ohm
#define R0 10000.0      // NTC nominal resistance at 25°C
#define T0 298.15       // 25°C in Kelvin


double r_to_t(double r_t);


int main(void) {
  pynq_init();
  adc_init();
  buttons_init();


  double v_out;
  double r_t;
  double temperature;


  while (!get_button_state(BUTTON0)) {
    v_out = adc_read_channel(ADC0);


    if (v_out <= 0.0) {
      printf("Error: v_out too low: %f\n", v_out);
    } else {
      r_t = (V_REF - v_out) * R2 / v_out;


      temperature = r_to_t(r_t);


      printf("V_out: %.3f V, R_NTC: %.2f ohm, Temperature: %.2f C\n",
             v_out, r_t, temperature);
    }


    sleep_msec(1000);
  }

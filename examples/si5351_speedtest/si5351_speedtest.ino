// si5351-speedtest.ino

#include <Wire.h>
#include <si5351.h>

#define TWI_SPEED 400000L


si5351 si;

void setup()
{
	Serial.begin(9600);
	Serial.println("SI5351 Speed test");

	Wire.begin();
	
	TWBR = ((F_CPU / TWI_SPEED) - 16) >> 1;								// Set I2C speed to 400kHz

	si.begin();
	si.set_xtal(27000000, -9);											// XTAL = 27.000 MHz with -9ppm correction
	si.pll_integer_config(PLL_A, 30);									// PLL_A = 30 * XTAL
	si.clk_config(CLK0, (SRC_PLL << CLKx_SRC) | (IDRV_8 << CLKx_IDRV)); // CLK0 powerd up, fractional mode, MultiSynth0, PLL A, not inverted, 8mA drive
	si.set_phase(CLK0, 0);												// no phase offset for CLK0
	si.clk_enable(CLK0);												// enable CLK0 output

	uint32_t timer_start, timer_stop;
	uint32_t i;

	Serial.println();
	Serial.println("Performing 10000 iterations of set_frequency()");

	timer_start = micros();
	for(i=0;i<10000;i++)
		si.set_frequency(CLK0,14285000+i);
	timer_stop = micros();

	Serial.print("Duration: ");
	Serial.print(timer_stop - timer_start);
	Serial.println(" us");
	Serial.print("Average per call: ");
	Serial.print((float)(timer_stop - timer_start)/10000);
	Serial.println(" us");

	Serial.println();
	Serial.println("Performing 10000 iterations of simple_set_frequency()");

	timer_start = micros();
	for(i=0;i<10000;i++)
		si.simple_set_frequency(CLK0,14285000+i);
	timer_stop = micros();

	Serial.print("Duration: ");
	Serial.print(timer_stop - timer_start);
	Serial.println(" us");
	Serial.print("Average per call: ");
	Serial.print((float)(timer_stop - timer_start)/10000);
	Serial.println(" us");
}

void loop()
{
}

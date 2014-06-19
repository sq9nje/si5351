#include <si5351.h>
#include <Wire.h>

si5351 si;

void setup()
{
	Wire.begin();

	si.begin();
	si.set_xtal(27000000);													// XTAL = 27.000 MHz
	si.clk_config(CLK0, (SRC_XTAL << CLKx_SRC) | (IDRV_8 << CLKx_IDRV)); 	// CLK0 powerd up, not inverted, 8mA drive, output XTAL

	si.clk_enable(CLK0);													// enable CLK0 output
}

void loop()
{
}

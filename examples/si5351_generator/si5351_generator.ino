// si5351-generator.ino
#include <Wire.h>
#include <si5351.h>

// Read a line of text into a buffer
uint8_t readline(char readch, char *buffer, uint8_t len)
{
  static uint8_t pos = 0;
  uint8_t rpos;
  
  if (readch > 0) {
    switch (readch) {
      case '\n': // Ignore new-lines
        break;
      case '\r': // Return on CR
        rpos = pos;
        pos = 0;  // Reset position index ready for next time
        return rpos;
      default:
        if (pos < len-1) {
          buffer[pos++] = readch;
          buffer[pos] = 0;
        }
    }
  }
  // No end of line has been found, so return 0.
  return 0;
}


si5351 si;

void setup()
{
	Serial.begin(9600);
	Serial.println("SI5351 Gnerator");

	Wire.begin();

	si.begin();
	si.set_xtal(27000000, -8);											// XTAL = 27.000 MHz with -9ppm correction
	si.pll_integer_config(PLL_A, 30);									// PLL_A = 30 * XTAL
	si.clk_config(CLK0, (SRC_PLL << CLKx_SRC) | (IDRV_8 << CLKx_IDRV)); // CLK0 powerd up, fractional mode, MultiSynth0, PLL A, not inverted, 8mA drive
	si.set_phase(CLK0, 0);												// no phase offset for CLK0
	si.clk_enable(CLK0);												// enable CLK0 output
}

void loop()
{
	static char buffer[32];

	if(readline(Serial.read(), buffer, 32))
	{
		si.set_frequency(CLK0, atol(buffer));
		Serial.print(">> OK ");
		Serial.print(atol(buffer));
		Serial.println(" Hz");
	}  
}

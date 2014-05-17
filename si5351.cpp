/**************************************************************************/
/*! 
    @file     si5351.cpp
    @author   Przemek Sadowski SQ9NJE
	@license  BSD (see license.txt)
	
	@section  HISTORY

    v1.0  - First release
*/
/**************************************************************************/

#include "si5351.h"
#include <Serial.h>


/**************************************************************************/
/*! 
    @brief  Performs basic initialisation of SI5351
*/
/**************************************************************************/
void si5351::begin()
{
	write_register(0x09, 0xFF);		// disable OEB pin
	write_register(0x0F, 0x00);		// XTAL as input for both PLLs
	write_register(0xB7, 0xC0);		// 10pF	XTAL load capacitance
	write_register(0x03, 0xFF);		// disable all outputs
}

/**************************************************************************/
/*! 
    @brief  Writes a value to a single register

    @param[in]	addr
    			Register address. Some constants defined in si5351.h
    @param[in]	value
    			Value to write.
*/
/**************************************************************************/
void si5351::write_register(uint8_t addr, uint8_t value)
{
	Wire.beginTransmission(_addr);
	Wire.write(addr);
	Wire.write(value);
	Wire.endTransmission();
}

/**************************************************************************/
/*! 
    @brief  Writes a continuous block of registers

    @param[in]	addr
    			Address of the first register to write.
    @param[in]	value
    			Pointer to the data block to transfer.
    @param[in]	len
    			Number of registers to write
*/
/**************************************************************************/
void si5351::write_block(uint8_t addr, uint8_t* value, uint8_t len)
{
	uint8_t i;
	Wire.beginTransmission(_addr);
	Wire.write(addr);
	for(i=0;i<len;i++)
		Wire.write(value[i]);
	Wire.endTransmission();
}

/**************************************************************************/
/*! 
    @brief  Reads a single register

    @param[in]	addr
    			Register address. Some constants defined in si5351.h
	@return		Register value.
*/
/**************************************************************************/
uint8_t si5351::read_register(uint8_t addr)
{
	Wire.beginTransmission(_addr);
	Wire.write(addr);
	Wire.requestFrom(_addr, (uint8_t)1);

	return Wire.read();
}

/**************************************************************************/
/*! 
    @brief  Sets the x-tal oscillator frequency

    @param[in]	xtal
    			X-tal frequency in Hz
*/
/**************************************************************************/
void si5351::set_xtal(uint32_t xtal)
{
	_xtal = xtal;
}

/**************************************************************************/
/*! 
    @brief  Sets the x-tal oscillator frequency

    @param[in]	xtal
    			X-tal frequency in Hz
    @param[in]	ppm
    			Calibration factor in ppm
*/
/**************************************************************************/
void si5351::set_xtal(uint32_t xtal, int ppm)
{
	_xtal = xtal;
	_xtal += xtal/1000000 * ppm;
}

/**************************************************************************/
/*! 
    @brief  Sets the PLL multiplication factor. Only integer mode supported

    @param[in]	pll
    			Which PLL to configure. Accepts PLL_A or PLL_B
    @param[in]	multiplier
    			Multiplication factor. No range checking for now
*/
/**************************************************************************/
void si5351::pll_integer_config(uint8_t pll, uint8_t multiplier)
{
	uint8_t config[] = {0x00, 0x01, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00};
	uint8_t t;
	uint32_t p1;

	p1 = 128*multiplier - 512;
	config[2] = (p1 & 0x00030000) >> 16;
	config[3] = (p1 & 0x0000FF00) >> 8;
	config[4] = (p1 & 0x000000FF);

	if(pll == PLL_A)
	{
		_mult_a = multiplier;
		t = read_register(0x16);
		write_register(0x16, t | (1 << FBx_INT));	// integer mode for PLL A
	}

	if(pll == PLL_B)
	{
		_mult_b = multiplier;
		t = read_register(0x17);
		write_register(0x17, t | (1 << FBx_INT));	// integer mode for PLL B
	}

	write_block(pll, config, 8);					// write MultiSynth config
	write_register(0xB1, 0xA0);						// reset both PLLs
}

/**************************************************************************/
/*! 
    @brief  Configures an output channel

    @param[in]	clk
    			Number of the configured channel. Accepts CLK0, CLK1 or CLK2
    @param[in]	config
    			Configuration byte. Some constants defined in si5351.h. 
    			Refer to AN619 for description of registers 16 - 23.
*/
/**************************************************************************/
void si5351::clk_config(uint8_t clk, uint8_t config)
{
	if(config & (1<<MSx_SRC))
		_clk_src = _clk_src & ~(1<<clk) | (1<<clk);

	write_register(CLK0_CFG + clk, config);
}

/**************************************************************************/
/*! 
    @brief  Disable output

    @param[in]	clk
    			Number of the configured channel. Accepts CLK0, CLK1 or CLK2
*/
/**************************************************************************/
void si5351::clk_disable(uint8_t clk)
{
	write_register(CLK_EN, read_register(CLK_EN) | (1 << clk));
}

/**************************************************************************/
/*! 
    @brief  Enable output

    @param[in]	clk
    			Number of the configured channel. Accepts CLK0, CLK1 or CLK2
*/
/**************************************************************************/
void si5351::clk_enable(uint8_t clk)
{
	write_register(CLK_EN, read_register(CLK_EN) & ~(1 << clk));
}

/**************************************************************************/
/*! 
    @brief  Set phase offset

    @param[in]	clk
    			Number of the configured channel. Accepts CLK0, CLK1 or CLK2
    @param[in]	phase
    			Phase offset. Refer to AN619 for description 
    			of registers 165 - 170.
*/
/**************************************************************************/
void si5351::set_phase(uint8_t clk, uint8_t phase)
{
	write_register(CLK0_PHOFF + clk, phase & 0x7F);
}

/**************************************************************************/
/*! 
    @brief  Set output frequency. Automaticaly selects integer/fractional
    		mode as appropriate.

    @param[in]	clk
    			Number of the configured channel. Accepts CLK0, CLK1 or CLK2
    @param[in]	freq
    			Output frequency in Hz.
*/
/**************************************************************************/
void si5351::set_frequency(uint8_t clk, uint32_t freq)
{
	float divider;
	uint32_t a, b, c, p1, p2;
	uint8_t config[8];

	if(_clk_src & (1<<clk))
		divider = (_xtal*_mult_b)/(float)freq;
	else
		divider = (_xtal*_mult_a)/(float)freq;

	a = (int32_t)divider;
	divider -= a;

	if(divider == 0) {
		write_register(MS_0 + clk, read_register(MS_0 + clk) | (1<<MSx_INT));
		c = 1;
		b = 0;
	}
	else {
		write_register(MS_0 + clk, read_register(MS_0 + clk) & ~(1<<MSx_INT));
		farey(divider, b, c);
	}

	p1 = 128*a + (128 * b)/c - 512;
	p2 = 128*b - c*((128 * b)/c);

	config[0] = (c & 0x0000FF00) >> 8;
	config[1] = (c & 0x000000FF);
	config[2] = (p1 & 0x00030000) >> 16;
	config[3] = (p1 & 0x0000FF00) >> 8;
	config[4] = (p1 & 0x000000FF);
	config[5] = ((p2 & 0x000F0000) >> 16) | (c & 0x000F0000) >> 12;
	config[6] = (p2 & 0x0000FF00) >> 8;
	config[7] = (p2 & 0x000000FF);

	write_block(MS_0 + clk*8, config, 8);
}

/**************************************************************************/
/*! 
    @brief  Find best rational approximation using Farey series. Refer to
    http://www.johndcook.com/blog/2010/10/20/best-rational-approximation 
    for algorithm description.

    @param[in]	alpha
    			Real number that is to be approximated. Must be < 1.
    @param[out]	x
    			Numerator
    @param[out]	y
    			Denominator
*/
/**************************************************************************/
void si5351::farey(float alpha, uint32_t &x, uint32_t &y)
{
	uint32_t p, q, r, s;
	float mediant;

	p = 0; q = 1;
	r = 1; s = 1;

	while(q <= FAREY_N && s <= FAREY_N)
	{
		//mediant = (float)(p + r)/(q + s);
		if(alpha*(q+s) == (p+r))
			if( (q+s) <= FAREY_N) {
				x = p + r;
				y = q + s;
				return;
			}
			else if(s > q) {
				x = r;
				y = s;
				return;
			}
			else {
				x = p;
				y = q;
				return;
			}
		else if(alpha*(q+s) > (p+r)) {
			p += r;
			q += s;
		}
		else {
			r += p;
			s += q;
		}
	}

	if(q > FAREY_N) {
		x = r;
		y = s;
		return;
	}
	else {
		x = p;
		y = q;
		return;
	}
}

/*void si5351::farey(float alpha, uint32_t &x, uint32_t &y)
{
	uint32_t p, q, r, s;
	float mediant;

	p = 0; q = 1;
	r = 1; s = 1;

	while(q <= FAREY_N && s <= FAREY_N)
	{
		mediant = (float)(p + r)/(q + s);
		if(alpha == mediant)
			if( (q+s) <= FAREY_N) {
				x = p + r;
				y = q + s;
				return;
			}
			else if(s > q) {
				x = r;
				y = s;
				return;
			}
			else {
				x = p;
				y = q;
				return;
			}
		else if(alpha > mediant) {
			p += r;
			q += s;
		}
		else {
			r += p;
			s += q;
		}
	}

	if(q > FAREY_N) {
		x = r;
		y = s;
		return;
	}
	else {
		x = p;
		y = q;
		return;
	}
}*/
/**************************************************************************/
/*! 
    @file     si5351.h
    @author   Przemek Sadowski SQ9NJE
	@license  BSD (see license.txt)
	
	@section  HISTORY

    v1.0  - First release
*/
/**************************************************************************/

#ifndef __SI5351_H__
#define __SI5351_H__

#include <utility/twi.h>
#include <Wire.h>

//#define SI5351_ADDR		0x63	// I2C address of the Mouser batch of ICs
#define SI5351_ADDR		0x60	// normal I2C address

#define PLL_A			26
#define PLL_B			34

#define MS_0			42
#define CLK0_PHOFF		165
#define CLK0_CFG		0x10

#define CLK_EN 			0x03

#define CLK0			0
#define CLK1			1
#define CLK2			2

#define FBx_INT			6
#define CLKx_PDN		7
#define MSx_INT			6
#define	MSx_SRC			5
#define	CLKx_INV		4
#define CLKx_SRC		2
#define CLKx_IDRV		0

#define IDRV_8			3
#define IDRV_6			2
#define IDRV_4			1
#define IDRV_2			0

#define SRC_A			0
#define SRC_B			1

#define SRC_XTAL		0
#define SRC_CLKIN		1
#define SRC_PLL			3

#define FAREY_N			1048575


class si5351
{
	private:
		uint8_t _mult_a, _mult_b;
		uint8_t _clk_src;
		uint8_t _addr;
		uint32_t _xtal;
		int _ppm;

		void farey(float alpha, uint32_t &x, uint32_t &y);

	public:
		si5351(uint8_t addr = SI5351_ADDR) { _addr = addr;}

		void write_register(uint8_t addr, uint8_t value);
		void write_block(uint8_t addr, uint8_t* value, uint8_t len);
		uint8_t read_register(uint8_t addr);

		void begin();
		void set_xtal(uint32_t xtal);
		void set_xtal(uint32_t xtal, int ppm);
		void pll_integer_config(uint8_t pll, uint8_t multiplier);
		void clk_config(uint8_t clk, uint8_t config);
		void clk_disable(uint8_t clk);
		void clk_enable(uint8_t clk);
		void set_phase(uint8_t clk, uint8_t phase);
		void set_frequency(uint8_t clk, uint32_t freq);
		void simple_set_frequency(uint8_t clk, uint32_t freq);
};

#endif /* __SI5351_H__ */
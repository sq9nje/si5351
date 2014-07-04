/*******************************************/
/*     A simple generator with Si5351A     */
/*******************************************/
/* The frequency is displayed on 16x2 LCD  */
/* the settings are controlled with a      */
/* standard mechanical encoder,            */
/* the frequency setting step can be       */ 
/* changed with a button.                  */
/*******************************************/

#include <Wire.h>
#include <si5351.h>
#include <Rotary.h>
#include <LiquidCrystal.h>

#define F_MULT       4                      // Output frequency multiplier. If set to 1 display frq. = output frq. 
#define F_MIN        1000000L               // Lower frequency limit
#define F_MAX        30000000L              // Upper frequency limit

#define ENCODER_A    3                      // Encoder pin A
#define ENCODER_B    2                      // Encoder pin B
#define ENCODER_BTN  4                      // Button pin

LiquidCrystal lcd(5, 6, 7, 8, 9, 10);       // LCD - connections: RS - pin 5, E - pin 6, D4 - D7 - pins 7 - 10
si5351 synt;                                // Si5351
Rotary r = Rotary(ENCODER_A, ENCODER_B);

volatile uint32_t frequency = 14285000L;    // Power on frequency setting
volatile uint32_t radix = 100;              // Initial frequency change step

boolean changed_f = 0;

/**************************************/
/* Interrupt service routine for      */
/* encoder frequency change           */
/**************************************/
ISR(PCINT2_vect) {
  unsigned char result = r.process();
  if (result == DIR_CW)
    set_frequency(1);
  else if (result == DIR_CCW) 
    set_frequency(-1);
}

/**************************************/
/* Read the button with debouncing    */
/**************************************/
boolean get_button()
{
  if(!digitalRead(ENCODER_BTN))
  {
    delay(20);
    if(!digitalRead(ENCODER_BTN))
    {
      while(!digitalRead(ENCODER_BTN));
      return 1;
    }
  }
  return 0;
}

/**************************************/
/* Change the frequency               */
/* dir = 1    Increment               */
/* dir = -1   Decrement               */
/**************************************/
void set_frequency(short dir)
{
  if(dir == 1)
    frequency += radix;
  if(dir == -1)
    frequency -= radix;
 
  if(frequency > F_MAX)
    frequency = F_MAX;
  if(frequency < F_MIN)
    frequency = F_MIN;
 
  changed_f = 1;  
}

/**************************************/
/* Displays the frequency             */
/**************************************/
void display_frequency()
{
  uint16_t f, g;
  
  lcd.setCursor(4, 0);
  f = frequency / 1000000;
  if(f<10)
    lcd.print(' ');
  lcd.print(f);
  lcd.print('.');
  f = (frequency % 1000000)/1000;
  if(f<100)
    lcd.print('0');
  if(f<10)
    lcd.print('0');
  lcd.print(f);
  lcd.print('.');
  f = frequency % 1000;
  if(f<100)
    lcd.print('0');
  if(f<10)
    lcd.print('0');
  lcd.print(f);
  lcd.print("Hz");
}

/**************************************/
/* Displays the frequency change step */
/**************************************/
void display_radix()
{
  lcd.setCursor(10, 1);
  switch(radix)
  {
    case 10:
      lcd.print("  10");
      break;
    case 100:
      lcd.print(" 100");
      break;
    case 1000:
      lcd.print("  1k");
      break;
    case 10000:
      lcd.print(" 10k");
      break;
    case 100000:
      lcd.print("100k");
      break;
  }
  lcd.print("Hz");
}

void setup()
{
  lcd.begin(16, 2);                                                    // Initialize and clear the LCD
  lcd.clear();
  
  Wire.begin();
  synt.begin();
  synt.set_xtal(27000000, -8);						// XTAL = 27.000 MHz with -9ppm correction
  synt.pll_integer_config(PLL_A, 35);					// PLL_A = 30 * XTAL
  synt.clk_config(CLK0, (SRC_PLL << CLKx_SRC) | (IDRV_8 << CLKx_IDRV)); // CLK0 powerd up, fractional mode, MultiSynth0, PLL A, not inverted, 8mA drive
  synt.set_phase(CLK0, 0);						// no phase offset for CLK0
  synt.clk_enable(CLK0);                                                // Enable CLK0 output
  synt.simple_set_frequency(CLK0, frequency*F_MULT);                    // Set the frequency
    
  pinMode(ENCODER_BTN, INPUT_PULLUP);                                   // Input with internal pullup for the button
  
  PCICR |= (1 << PCIE2);                                                // Enable pin change interrupt for the encoder
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();
  
  display_frequency();                                                  // Update the display
  display_radix();
}

void loop()
{
  // Update the display if the frequency has been changed
  if(changed_f)
  {
    display_frequency();
    synt.simple_set_frequency(CLK0, frequency*F_MULT);
    changed_f = 0;
  }
  
  // Button press changes the frequency change step
  if(get_button())
  {
    switch(radix)
    {
      case 10:
        radix = 100;
        break;
      case 100:
        radix = 1000;
        break;
      case 1000:
        radix = 10000;
        break;
        case 10000:
        radix = 100000;
        break;
      case 100000:
        radix = 10;
        break;
    }
    display_radix();
  }   
}

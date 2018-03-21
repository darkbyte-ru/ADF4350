# ADF4350

Communication library to interface with Analog Devices ADF4350 PLL IC.

## Getting started

In your Arduino sketch, you'll want to include the SPI library in addition to this code:

#include <SPI.h>
#include <ADF4350.h>

#define PLL_LE_PIN 8
ADF4350 PLL(PLL_LE_PIN);

void setup()
{
  //init ADF4350 with 10MHz reference and tune to 432MHz
  PLL.initialize(432000, 10);
}


Full example code can be found in example directory. adf4350-advanced example need to be connected 2x16 LCD display via i2c adapter (pcf8574 for example) and rotary encoder with push button. Also that example read heating status of FE-5680A rubidium frequency standard. Remove HEAT_PIN if you using other reference frequency source.

### Important note

The ADF4350 works with 3.3V logic levels, not 5V. Be careful if you're using an Arduino Uno or similar!

## Implemented features

Self-explanatory functions...

* `ADF4350::initialize(int frequency, int refClk)` -- initializes PLL with given frequency (KHz) and reference clock frequency (MHz).
* `ADF4350::getFreq()` -- returns current frequency
* `ADF4350::setFreq(long freq)` -- sets PLL to output new frequency `freq` (in KHz).

Functions you should use after consulting datasheet:

* `ADF4350::setFeedbackType(bool feedback)` -- fundamental vs. divided feedback
* `ADF4350::powerDown(bool pd)` -- power down the VCO (or not).
* `ADF4350::rfEnable(bool rf)` -- enable/disable output on the main RF output.
* `ADF4350::setRfPower(int pow)` -- `pow` should be 0, 1, 2, or 3, corresponding to -4, -1, 3, or 5 dBm.
* `ADF4350::auxEnable(bool aux)` -- enable/disable output on the auxilary output.
* `ADF4350::setAuxPower(int pow)` -- set auxiliary power output. Again, `pow` should be 0, 1, 2, or 3, corresponding to -4, -1, 3, or 5 dBm. 

#include <SPI.h>
#include <ADF4350.h>

#define PLL_REFERENCE_FREQ 10 //MHz clock 
#define PLL_LE_PIN 8

ADF4350 PLL(PLL_LE_PIN);

void setup()
{
  Serial.begin(115200);

  //init ADF4350 with 10MHz reference and tune to 432MHz
  PLL.initialize(432000, PLL_REFERENCE_FREQ);

  //default power is 0, which means -4dBm
  //PLL.setRfPower(0);
}

void loop()
{

}


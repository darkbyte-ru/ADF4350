/* 
   ADF4350.cpp - ADF4350 PLL Communication Library
   Based on Neal Pisenti code

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   aunsigned long with this program.  If not, see <http://www.gnu.org/licenses/>.


 */

#include "Arduino.h"
#include "SPI.h"
#include <ADF4350.h>



/* CONSTRUCTOR */

// Constructor function; initializes communication pinouts

ADF4350::ADF4350(byte ssPin) {
    _ssPin = ssPin;
    pinMode(_ssPin, OUTPUT);
    digitalWrite(_ssPin, HIGH);
}

// Initializes a new ADF4350 object, with refClk (in Mhz), and initial frequency.
void ADF4350::initialize(long freq, int refClk = 10)
{
    SPI.begin();                          // Init SPI bus
    SPI.setDataMode(SPI_MODE0);           // CPHA = 0  Clock positive
    SPI.setBitOrder(MSBFIRST);
    delay(10);

    _refClk = (float)refClk;
    _phase = 1;

    _feedbackType = 1;
    _powerdown = 0;
    _auxEnabled = 0;
    _rfEnabled = 1;

    // default power = -4dbm
    _auxPower = 0;
    _rfPower = 0;

    // sets register values which don't have dynamic values...
    ADF4350::setR1();
    ADF4350::setR3();
    ADF4350::setR5();

    ADF4350::setFreq(freq);
}

// gets current frequency setting
long ADF4350::getFreq(){
    return _freq*1000;
}

void ADF4350::setFreq(long freq)
{

    _freq = (float)freq / 1000.0;

    if (_freq < 68.75) {
      _divider = 6;
    }else
    if (_freq < 137.5) {
      _divider = 5;
    }else
    if (_freq < 275)  {
      _divider = 4;
    }else
    if (_freq < 550)  {
      _divider = 3;
    }else
    if (_freq < 1100) {
      _divider = 2;
    }else
    if (_freq < 2200) {
      _divider = 1;
    }else
    if (_freq >= 2200) {
      _divider = 0;
    }

    MOD = (_refClk / OutputChannelSpacing);
    INTA = (_freq * pow(2,_divider)) / _refClk;
    FRACF = (((_freq * pow(2,_divider)) / _refClk) - INTA) * MOD;
    FRAC = round(FRACF); 

    ADF4350::update();
}



// updates dynamic registers, and writes values to PLL board
void ADF4350::update()
{
    // updates registers with dynamic values...
    ADF4350::setR0();
    ADF4350::setR2();
    ADF4350::setR3();

    //for debug
    ADF4350::setR1();
    ADF4350::setR4();

    // writes registers to device
    for (int i = 5; i >= 0; i--){
        ADF4350::WriteRegister32(_registers[i]);
    }
}

void ADF4350::WriteRegister32(const uint32_t value)
{
  digitalWrite(_ssPin, LOW);
  for (int i = 3; i >= 0; i--)
    SPI.transfer((value >> 8 * i) & 0xFF);
  digitalWrite(_ssPin, HIGH);
  //digitalWrite(_ssPin, LOW);
}


void ADF4350::setFeedbackType(bool feedback){
    _feedbackType = feedback;
}

void ADF4350::powerDown(bool pd){
    _powerdown = pd;
    ADF4350::update();

}

void ADF4350::rfEnable(bool rf){
    _rfEnabled = rf;
    ADF4350::update();
}

// CAREFUL!!!! pow must be 0, 1, 2, or 3... corresponding to -4, -1, 3, 5 dbm.
void ADF4350::setRfPower(int pow){
    if(pow > 3) pow = 3;
    _rfPower = pow;
    ADF4350::update();
}

void ADF4350::auxEnable(bool aux){
    _auxEnabled = aux;
    ADF4350::update();
}

// CAREFUL!!!! pow must be 0, 1, 2, or 3... corresponding to -4, -1, 3, 5 dbm.
void ADF4350::setAuxPower(int pow){
    if(pow > 3) pow = 3;
    _auxPower = pow;
    ADF4350::update();
}

// REGISTER UPDATE FUNCTIONS

void ADF4350::setR0()
{
    _registers[0] =
        (INTA << 15)  //31-15 - 16-bit integer value
        + (FRAC << 3); //14-3 - 12-bit fractional value
        + 0;
}

void ADF4350::setR1()
{
    _registers[1] = 
        //31-28 - reverved
        (1UL << 27) //27 - prescaler
        + ((unsigned long)_phase << 15) //26-15 - 12-bit phase value
        + ((unsigned long)(MOD) << 3) //14-3 - 12-bit modulus value
        + 1;
}

void ADF4350::setR2()
{
    _registers[2] =
        //31 - reserved
        +(0b11UL << 29)//30-29 - low noise and low spur modes
        +(0b110UL << 26) //28-26 - muxout
        //25 - ref doubler
        //24 - ref divider
        + (1UL << 14) //23-14 - 10-bit R counter
        //13 - double buf
        + (0b1110UL << 9) //12-9 - charge pump current setting
        //8 - LDF
        //7 - LDP
        + (1UL << 6) //6 - PD polarity
        +((unsigned long)_powerdown << 5) //5 - PD
        //4 - CP three-state
        //3 - counter reset
        + 2;
}

void ADF4350::setR3()
{
    /*
    31-21 - reserved
    20-19 - reverved
    18 - CSR
    17 - reverved
    16-15 - CLK DIV mode
    14-3 - 12-bit clock divider value
    */
   _registers[3] = ((150UL) << 3) + 3;
}

void ADF4350::setR4()
{
    _registers[4] = 
        //31-24 - reserved
        ((unsigned long)_feedbackType << 23) + //23 - feedback select
        ((unsigned long)_divider << 20) +//22-20 - divider select
        (200UL << 12) + //19-12 - 8-bit band select 
        //(0UL << 9) + //11 - VCO power down
        //10 - Mute till lock detect
        //9 - AUX output select
        ((unsigned long)_auxEnabled << 8) + //8 - AUX output enable
        ((unsigned long)_auxPower << 6) + //7-6 - AUX output power
        ((unsigned long)_rfEnabled << 5) + //5 - RF output enable
        ((unsigned long)_rfPower << 3) + //4-3 RF output power
        4;
}

void ADF4350::setR5()
{
    _registers[5] = 
        //31-24 - reserved
        (1UL << 22) //23-22 - LD pin mode - digital lock detect
        //21 - reverved
        + (0b11UL << 19) //20-19 - reserved (high)
        //18-3 - reverved
        + 5; 
} 

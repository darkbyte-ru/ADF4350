/* 
   ADF4350.h - ADF4350 PLL Communication Library
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

#ifndef ADF4350_h
#define ADF4350_h

#include "Arduino.h"

class ADF4350
{
    public: 
        // Constructor function. 
        // Creates PLL object, with given SS pin
        ADF4350(byte);

        // Initialize with initial frequency, refClk (defaults to 10Mhz); 
        void initialize(long, int);


        // powers down the PLL/VCO
        void powerDown(bool);
        void setRfPower(int);
        void setAuxPower(int);
        void auxEnable(bool);
        void rfEnable(bool);
        void setInt(int);
        void setFeedbackType(bool);
        
        long getFreq();
        void setFreq(long);

        void update();


    private:
        byte _ssPin;

        bool _powerdown, _auxEnabled, _rfEnabled, _feedbackType;

        unsigned long _phase, _int, _divider, _auxPower, _rfPower;
        float _refClk, _freq;

        uint32_t _registers[6];
        double OutputChannelSpacing = 0.01;

        double FRACF;
        unsigned int long INTA, MOD, FRAC;

        // function to write data to register.
        void setR0();
        void setR1();
        void setR2();
        void setR3();
        void setR4();
        void setR5();
        void WriteRegister32(const uint32_t value);



};
 

#endif

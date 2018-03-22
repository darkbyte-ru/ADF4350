#include <SPI.h>
#include <ADF4350.h>
#define PLL_LE_PIN 8
ADF4350 PLL(PLL_LE_PIN);

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//i2c address and pin config for pcf8574 board, maybe differ
LiquidCrystal_I2C lcd(0x20, 4, 5, 6, 0, 1, 2, 3, 7, NEGATIVE);

#include <Encoder.h>
Encoder encoder(3, 2); //pins with interrupt support prefer
long oldPosition  = 0;
#define ENCODER_SW_PIN 5
#define ENCODER_CLICK_STEP 4.0 //steps between fixed position of encoder

#define LOCK_PIN 4 //lock status from ADF4350 via MUXOUT pin
#define HEAT_PIN 6 //heat status from FE-5680A rubidium frequency standard
bool pll_lock = false;
bool rubid_heating = false;

#include <avr/eeprom.h>
#define EEPROM_WRITE_TIMEOUT 60000UL //save current config to EEPROM after 60 sec of last change
#define EEPROM_ADDR_FREQ 0
#define EEPROM_ADDR_CURPOS 5
#define EEPROM_ADDR_POWER 6
unsigned long eeprom_write_timeout = 0;

#define PLL_REFERENCE_FREQ 10 //MHz clock
#define PLL_FREQ_MIN 137000 //kHz
#define PLL_FREQ_MAX 4400000 //kHz

#define PLL_POWER_MIN 0
#define PLL_POWER_MAX 3

#define CURSOR_POS_MIN 0
#define CURSOR_POS_MAX 7

unsigned long freq;
bool encode_sw_state;
unsigned long encode_debounce = 0;
bool cursor_blinking = false;
int8_t cursor_pos = 6;
int8_t pll_rf_power = 0;

void setup()
{
  Serial.begin(115200);

  lcd.begin(16, 2);
  lcd.backlight();
  lcd.home();
  lcd.print("Hello, world!");

  //spike for ADF4350 cold start
  delay(5000);

  freq = eeprom_read_dword(EEPROM_ADDR_FREQ);
  if (freq >= PLL_FREQ_MIN && freq <= PLL_FREQ_MAX) {
    Serial.print(F("Readed freq "));
    Serial.println(freq);

    cursor_pos = eeprom_read_byte(EEPROM_ADDR_CURPOS);
    if (cursor_pos > CURSOR_POS_MAX)cursor_pos = 0;

    pll_rf_power = eeprom_read_byte(EEPROM_ADDR_POWER);
    if (pll_rf_power > PLL_POWER_MAX)pll_rf_power = 0;

  } else freq = 432000;

  PLL.initialize(freq, PLL_REFERENCE_FREQ);
  PLL.setRfPower(pll_rf_power);

  pinMode(HEAT_PIN, INPUT);
  pinMode(LOCK_PIN, INPUT);
  pinMode(ENCODER_SW_PIN, INPUT);
  digitalWrite(ENCODER_SW_PIN, HIGH);

  printLCD();
  lcd.cursor();
}

void printLCD()
{
  //long operation with screen blink
  //lcd.clear();

  //split 123456 to 123.456
  unsigned long part1 = freq / 1000;
  unsigned long part2 = freq - part1 * 1000;

  lcd.setCursor(0, 0);
  lcd.print("F =");
  if (part1 < 1000)lcd.print(" ");
  lcd.print(part1);
  lcd.print(".");
  if (part2 < 100)lcd.print("0");
  if (part2 < 10)lcd.print("0");
  lcd.print(part2);
  lcd.print(".000 ");

  lcd.setCursor(0, 1);
  lcd.print("P =");

  switch (pll_rf_power) {
    case 0:
      lcd.print("-4");
      break;
    case 1:
      lcd.print("-1");
      break;
    case 2:
      lcd.print(" 3");
      break;
    case 3:
      lcd.print(" 5");
      break;
  }
  lcd.print("dBm");

  lcd.print(" ");

  if(rubid_heating){
    lcd.print("   HEAT");
  }else{
    if (!pll_lock){
      lcd.print("NO ");
    }else{
      lcd.print("   ");
    }
    lcd.print("LOCK");
  }

  if (cursor_pos <= 6) {
    lcd.setCursor(3 + cursor_pos + (cursor_pos > 3 ? 1 : 0), 0);
  } else {
    lcd.setCursor(0, 1);
  }
}

void loop()
{
  if(rubid_heating != digitalRead(HEAT_PIN)){
    rubid_heating = !rubid_heating;
    PLL.update(); //for sure about that
    printLCD();
  }
  
  if (digitalRead(LOCK_PIN) != pll_lock) {
    pll_lock = !pll_lock;
    printLCD();
  }

  if (digitalRead(ENCODER_SW_PIN) != encode_sw_state) {
    delay(10);//debounce
    encode_sw_state = digitalRead(ENCODER_SW_PIN);

    //LOW = pressed
    //Serial.println(encode_sw_state?"HIGH":"LOW");

    if (encode_sw_state == LOW) {
      if (cursor_blinking) {
        lcd.noBlink();
        cursor_blinking = false;
      } else {
        lcd.blink();
        cursor_blinking = true;
      }
    }

  }

  long newPosition = encoder.read();
  if (newPosition != oldPosition) {
    float steps = (newPosition - oldPosition) / ENCODER_CLICK_STEP;
    Serial.println(steps);
    if (steps > 0.5 || steps < -0.5) {

      if (cursor_blinking) {
        //if in digit position mode
        if (steps > 0) {
          cursor_pos++;
        } else {
          cursor_pos--;
        }
        if (cursor_pos > CURSOR_POS_MAX)cursor_pos = CURSOR_POS_MAX;
        if (cursor_pos < CURSOR_POS_MIN)cursor_pos = CURSOR_POS_MIN;

      } else {
        if (cursor_pos <= 6) {
          //change frequency
          if (steps > 0) {
            //max step 1GHz
            freq += (1000000UL / pow(10, cursor_pos)) * ceil(steps);
          } else {
            freq += (1000000UL / pow(10, cursor_pos)) * floor(steps);
          }

          if (freq < PLL_FREQ_MIN)freq = PLL_FREQ_MIN;
          if (freq > PLL_FREQ_MAX)freq = PLL_FREQ_MAX;

          PLL.setFreq(freq);
        } else {
          //change power
          if (steps > 0) {
            pll_rf_power++;
          } else {
            pll_rf_power--;
          }
          if (pll_rf_power > PLL_POWER_MAX)pll_rf_power = PLL_POWER_MAX;
          if (pll_rf_power < PLL_POWER_MIN)pll_rf_power = PLL_POWER_MIN;

          PLL.setRfPower(pll_rf_power);
        }
        eeprom_write_timeout = millis();
      }
      printLCD();
      oldPosition = newPosition;
      encode_debounce = millis();
    }
    if (millis() - encode_debounce > 250) {
      oldPosition = newPosition;
      encode_debounce = millis();
    }
    //Serial.println(newPosition);
  }

  if ((eeprom_write_timeout > 0) && (millis() - eeprom_write_timeout > EEPROM_WRITE_TIMEOUT)) {
    eeprom_write_timeout = 0;
    Serial.print(F("Write EEPROM"));
    eeprom_write_dword(EEPROM_ADDR_FREQ, freq);
    eeprom_write_byte(EEPROM_ADDR_CURPOS, cursor_pos);
    eeprom_write_byte(EEPROM_ADDR_POWER, pll_rf_power);
  }

}


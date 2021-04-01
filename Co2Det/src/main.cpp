#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <RotaryEncoder.h>
#include "wiring_private.h"
#include "button.h"
#include <FlashAsEEPROM.h>


#define Piezo 8
#define encoderButton 9
#define encoderDt 10
#define encoderClk 11
#define sensorRx 5
#define sensorTx 6

LiquidCrystal_I2C lcd(0x27,16,2);
RotaryEncoder encoder(encoderDt, encoderClk);
Button b1(encoderButton);

int ppm, temperature = 0;
int lPpm, lTemperature, lPos= 0;
bool settings, lSetting = false;
bool update = true;
bool alarm = false;
unsigned long endMute;

Uart co2Serial (&sercom0, sensorRx, sensorTx, SERCOM_RX_PAD_1, UART_TX_PAD_0);
void SERCOM0_Handler()
{
    co2Serial.IrqHandler();
}

void triggerButton() {
  if (alarm && !settings) {
    alarm = false;
    endMute = millis() + 300000;
    return;
  }
  settings = !settings;
  lcd.clear();
  update = true;
  if (settings) {
    lcd.setCursor(0, 0);
    lcd.print("Einstellungen: ");
    lcd.setCursor(0, 1);
    lcd.print("Limit: ");
  } else {
    EEPROM.write(0, (uint8_t) encoder.getPosition());
    EEPROM.commit();
    digitalWrite(Piezo, true);
    delay(1000);
    digitalWrite(Piezo, false);
  }
}

void updateEncoder() {
  if (!settings) return;
  encoder.tick();
  if (encoder.getPosition() > 150 ) encoder.setPosition(150);
  if (encoder.getPosition() < 50 ) encoder.setPosition(50);
}


 
void setup() {
  b1.init(&triggerButton);
  lcd.init();
  
  pinPeripheral(sensorRx, PIO_SERCOM_ALT);
  pinPeripheral(sensorTx, PIO_SERCOM_ALT);

  Serial.begin(9600);
  co2Serial.begin(9600);
  while (!co2Serial.availableForWrite());

  delay(1000);
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Preheate ...");
  unsigned long m = 0;

  char b [5];
  char lb = 1;
  do {
    m = 180000 - m;
    lcd.setCursor(0,1);
    sprintf(b, "%02d:%02d", m/60000, (m/1000)%60);
    if (b[4] != lb) {
      Serial.println(b);
      lcd.print(b);
    }
    lb = b[4];
    m = millis();
  } while (m < 180000);
  lcd.clear();
  
  pinMode(Piezo, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  attachInterrupt(digitalPinToInterrupt(10), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(11), updateEncoder, CHANGE);
  uint8_t er = EEPROM.read(0);
  encoder.setPosition(er > 150 ? 100 : er);
}


byte getCheckSum(byte *packet) {
  byte i;
  byte checksum = 0;
  for (i = 1; i < 8; i++) {
    checksum += packet[i];
  }
  checksum = 0xff - checksum;
  checksum += 1;
  return checksum;
}

void readSensor(int *ppm, int *temperature){
  if (co2Serial.available() >= 9) {
    byte response[9]; 
    memset(response, 0, 9);;
    co2Serial.readBytes(response, 9);
    byte check = getCheckSum(response);
    
    if (response[8] != check) {
      Serial.println("Fehler in der Übertragung!");
      delay(1000);
      co2Serial.read();
      return;
    }
    *ppm = 256 * (int)response[2] + response[3];
    *temperature = response[4] - 40;
  }

  if (millis() % 1000 < 10) {
    byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
    co2Serial.write(cmd, 9);
    Serial.println("Send");
    delay(10);
  }
}
 
void loop() {
  readSensor(&ppm, &temperature);
  if (temperature < 10) return;
  unsigned long m = millis();
  if (alarm) {
    unsigned short a = m % 150;
    digitalWrite(Piezo, a < 100);
  } else 
    digitalWrite(Piezo, false);
  digitalWrite(LED_BUILTIN, m < endMute);
  if (encoder.getPosition() *10 <= ppm && m > endMute) alarm = true;
  b1.tick();
  if (settings) {
    if (lPos != encoder.getPosition() || update) {
      lcd.setCursor(8, 1);
      lcd.print(encoder.getPosition()*10);
      lcd.print("ppm  ");
    }
  } else {
    if (lTemperature != temperature || lPpm != ppm || update) {
      lcd.setCursor(0, 0);
      lcd.print("Temparatur: ");
      lcd.print(temperature);
      lcd.print("C  ");
      lcd.setCursor(0, 1);
      lcd.print("Co2: ");
      lcd.print(ppm);
      lcd.print("ppm  ");
    }
}
  lTemperature = temperature; lPpm = ppm; lPos = encoder.getPosition(); lSetting = settings; update = false;
  delay(1);
  
}

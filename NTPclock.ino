/*
  GHF - 03-mar-19
  
  Shows an NTP-based digital clock on OLED display
  Connects to WiFi network using WiFiManager library
  MCU: D1 Mini
  Display: OLED 0.96" 128x64 pixel, I2C, SSD1306 controller

  To do: does not handle Daylight Saving, Time Zone is hard-coded

  Connections:
  OLED D2 (SDA) -> MCU pin D2 (SDA)
  OLED D1 (SCL) -> MCU pin D1 (SCL)
  OLED VCC -> MCU pin 3V (+3.3V)
  OLED GND -> MCU pin G

*/

#include <WiFiManager.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <U8g2lib.h>
#include <Time.h>

const int offsetFromGMT=-10800; // GMT-3
const int updInterval=60000; // NTP update interval in ms
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", offsetFromGMT, updInterval);
Ticker eachSec;
U8G2_SSD1306_128X64_NONAME_F_SW_I2C oled(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);
const char* weekDays[7]={"Dom","Lun","Mar","Mie","Jue","Vie","Sab"};
const char* monthNames[12]={"Ene","Feb","Mar","Abr","May","Jun",
                            "Jul","Ago","Sep","Oct","Nov","Dic"};

void updateWatch() {
  char tmp[16];
  unsigned long epoch=timeClient.getEpochTime();
  setTime(epoch);
  int tt=now();
  oled.clearBuffer();
  oled.setFont(u8g2_font_logisoso32_tf);
  sprintf(tmp,"%02d:%02d",hour(tt),minute(tt));
  oled.drawStr(5,35,tmp);
  oled.setFont(u8g2_font_logisoso16_tf);
  sprintf(tmp,"%02d",second(tt));
  oled.drawStr(100,27,tmp);
  sprintf(tmp,"%3s %02d-%3s-%02d", weekDays[weekday(tt)-1], 
     day(tt), monthNames[month(tt)-1], year(tt)-2000);
  oled.drawStr(0,60,tmp);
  oled.sendBuffer();
  int state=digitalRead(LED_BUILTIN);
  digitalWrite(LED_BUILTIN, !state);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  WiFiManager wifiManager;
  wifiManager.autoConnect();
  oled.begin();
  oled.setFont(u8x8_font_chroma48medium8_r);
  Serial.println("connected...");
  timeClient.begin();
  timeClient.update();
  eachSec.attach(1,updateWatch);
  pinMode(D3,OUTPUT);
}

void loop() {
}

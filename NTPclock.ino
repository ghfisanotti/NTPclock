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
#include <ESP8266HTTPClient.h>
#include <Ticker.h>
#include <U8g2lib.h>
#include <Time.h>
#include <ArduinoJson.h>

const int offsetFromGMT=-10800; // GMT-3
const int updInterval=600000; // NTP update interval in ms
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", offsetFromGMT, updInterval);
Ticker clockTimer, weatherTimer;
U8G2_SSD1306_128X64_NONAME_F_SW_I2C oled(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);
const char* weekDays[7]={"Dom","Lun","Mar","Mie","Jue","Vie","Sab"};
const char* monthNames[12]={"Ene","Feb","Mar","Abr","May","Jun",
                            "Jul","Ago","Sep","Oct","Nov","Dic"};
const String CITY_ID="3433955"; //Ciudad Autonoma de Buenos Aires
const String API_KEY="3755da794b13faa537eb7c8138c45c6d";
float weather_temp=0; //Ambient temperature
int weather_pressure=0; //Atmospheric pressure
int weather_humidity=0; //Ambient relative humidity
unsigned long weather_dt=0; //Time of last weather update
String weather_desc=""; //Weather conditions description
int weather_id=0; //Weather conditions ID
bool getWeatherNow=false;

void getWeather() {
  DynamicJsonBuffer jsonBuffer;
  HTTPClient http;
  String URL="http://api.openweathermap.org/data/2.5/weather?id=" + 
    CITY_ID + "&APPID=" + API_KEY + "&units=metric&lang=es";
  http.begin(URL);
  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
      JsonObject& json = jsonBuffer.parseObject(payload);
      weather_temp=json["main"]["temp"];
      weather_pressure=json["main"]["pressure"];
      weather_humidity=json["main"]["humidity"];
      weather_dt=json["dt"];
      weather_id=json["weather"][0]["id"];
      weather_desc=json["weather"][0]["description"].as<String>();
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  getWeatherNow=false;
}

void digitalClock() {
  char tmp[16];
  unsigned long tt=timeClient.getEpochTime();
  oled.clearBuffer();
  oled.setFont(u8g2_font_logisoso28_tf);
  sprintf(tmp,"%02d:%02d",hour(tt),minute(tt));
  oled.drawStr(12,45,tmp);
  oled.setFont(u8g2_font_logisoso16_tf);
  sprintf(tmp,"%02d",second(tt));
  oled.drawStr(100,40,tmp);
  //sprintf(tmp,"%3s %02d-%3s-%02d", weekDays[weekday(tt)-1], 
  //   day(tt), monthNames[month(tt)-1], year(tt)-2000);
  sprintf(tmp,"%3s/%02d   %02dC %02d%%", weekDays[weekday(tt)-1], 
       day(tt), round(weather_temp), weather_humidity);
  oled.setFont(u8g2_font_8x13B_mf);
  oled.drawStr(0,10,tmp);
  //sprintf(tmp,"%2dC %2d%% %4dhPa", 
  //        round(weather_temp), weather_humidity, weather_pressure); 
  sprintf(tmp,"%s", weather_desc.c_str()); 
  oled.drawStr( (128-oled.getStrWidth(tmp))/2, 62, tmp );
  oled.sendBuffer();
  // Blink builtin LED once per second except during night hours
  if (hour(tt)>=7 and hour(tt)<=23) {
    int state=digitalRead(LED_BUILTIN);
    digitalWrite(LED_BUILTIN, !state);
  } else {digitalWrite(LED_BUILTIN, HIGH);}
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D3,OUTPUT);
  Serial.begin(115200);
  timeClient.begin();
  oled.begin();
  oled.setFont(u8g2_font_logisoso16_tf);
  oled.clearBuffer();
  oled.drawStr(10,28,"Connecting");
  oled.drawStr(10,48,"to WiFi...");
  oled.sendBuffer();
  WiFiManager wifiManager;
  wifiManager.autoConnect();
  oled.clearBuffer();
  oled.drawStr(10,28,"Connected!");
  oled.drawStr(10,48,"Starting NTP");
  oled.sendBuffer();
  oled.clearBuffer();
  oled.drawStr(10,28,"NTP started!");
  oled.drawStr(1,48,"updating time");
  oled.sendBuffer();
  while (!timeClient.forceUpdate()) { delay(1000); }
  oled.clearBuffer();
  oled.drawStr(30,40,"Ready!");
  oled.sendBuffer();
  getWeather();
  clockTimer.attach(1,digitalClock); //display digital clock every second
  weatherTimer.attach(3600, [](){getWeatherNow=true;}); 
}

void loop() {
  if (!timeClient.update()) { Serial.println("NTP update failed!"); }
  if (getWeatherNow) { getWeather(); }
  delay(1000);
}

#include "Arduino.h"
#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Updater.h>

#include <ESPAsyncWiFiManager.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <SPIFFSEditor.h>

AsyncWebServer httpServer(80);
DNSServer dns;

#define DEBUG

#include <NTPClient.h>
#include <ArduinoJson.h>
#include <IPGeolocation.h>
String IPGeoKey = "b294be4d4a3044d9a39ccf42a564592b";
//#include "SimpleWeather.h"


#define FASTLED_INTERNAL
#define FASTLED_ESP8266_RAW_PIN_ORDER
//#define FASTLED_ESP8266_D1_PIN_ORDER
#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
#include "EEPROM.h"

#include "palette.h"
#include "config.h"

#include "Page_Admin.h"

// NTP Servers:

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 360000); //19800

void setup() {
    // put your setup code here, to run once:
    delay(3000);
    //Serial.begin(9600);
    FastLED.addLeds<WS2812B, 4, GRB>(leds, 60).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(0);
    fill_solid(leds, NUM_LEDS, bg);
    //fill_rainbow(leds, NUM_LEDS,4);
    FastLED.show();
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
    Serial.println("Wifi Setup Initiated");
    WiFi.setAutoConnect(true);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    AsyncWiFiManager wifiManager(&httpServer,&dns);
    wifiManager.setTimeout(180);
    if(!wifiManager.autoConnect(ESPNAME)) {
      delay(3000);
      ESP.reset();
      delay(5000);
      }
    Serial.println("Wifi Setup Completed");
    sendIP();

    // Admin page
    httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request ->send_P(200,"text/html", PAGE_AdminMainPage ); 
    });
    httpServer.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/style.css.gz", "text/css");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    httpServer.on("/microajax.js", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/microajax.js.gz", "text/plain");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    httpServer.on("/jscolor.js", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/jscolor.js.gz","text/plain");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    httpServer.on("/clock.html", HTTP_GET, send_clock_configuration_html);
    httpServer.on("/color.html", HTTP_GET, send_color_configuration_html);
    httpServer.on("/admin/clockconfig", HTTP_GET, send_clock_configuration_values_html);
    httpServer.on("/admin/colorconfig", send_color_configuration_values_html );
    httpServer.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){handleUpdate(request);});
    httpServer.on("/doUpdate", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,
                  size_t len, bool final) {handleDoUpdate(request, filename, index, data, len, final);}
    );
    httpServer.addHandler(new SPIFFSEditor("admin","admin"));
    httpServer.onNotFound(handleNotFound);
    httpServer.begin();

    IPGeolocation IPG(IPGeoKey);
    IPGeo I;
    IPG.updateStatus(&I);
    timeClient.setTimeOffset((int)I.offset*3600);
    
    MDNS.begin(ESPNAME);
    MDNS.addService("http", "tcp", 80);
    timeClient.begin();

    EEPROM.begin(512);

    if (EEPROM.read(109) != 4) saveDefaults();
    // Else read the parameters from the EEPROM
    else loadDefaults();

    fill_solid(leds, NUM_LEDS, bg);
    FastLED.show();
    gCurrentPalette = gGradientPalettes[config.gCurrentPaletteNumber];
    wdt_enable(WDTO_8S);
}

void loop() {
    timeClient.update();
    showTime(timeClient.getHours(),timeClient.getMinutes(),timeClient.getSeconds());
    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
    if(timeClient.getHours() == 3 && timeClient.getMinutes() == 0 && timeClient.getSeconds() == 0)
      ESP.restart();
    MDNS.update();
    yield();
}

void showTime(int hr, int mn, int sec) {
  if(mn==0) fill_solid(leds, NUM_LEDS, bg);
  if(( mn % config.rain == 0 && sec == 0)){
       effects();
    }
  colorwaves( leds, mn, gCurrentPalette);
  leds[hr%12*5]=hours;
  leds[hr%12*5+1]=hours;
  if(hr%12*5-1 > 0)
    leds[hr%12*5-1]=hours;
  else leds[59]=hours; 
  for(byte i = 0; i<60; i+=5){
    leds[i]= lines;  
  }
  leds[mn]= minutes;
  if(hr < config.switch_on || hr >= config.switch_off)
    LEDS.setBrightness(constrain(0,config.light_low,50)); // Set brightness to light_low during night - cools down LEDs and power supplies.
  else
    LEDS.setBrightness(constrain(config.light_high,10,255));
}

void effects(){
  for( int j = 0; j< 300; j++){
    fadeToBlackBy( leds, NUM_LEDS, 20);
    byte dothue = 0;
    for( int i = 0; i < 8; i++) {
      leds[beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
    FastLED.show();
    FastLED.delay(1000/UPDATES_PER_SECOND);
   }
  fill_solid(leds, NUM_LEDS, bg);
}

void handleNotFound(AsyncWebServerRequest *request){
  message= "Time: ";
  message+= String(timeClient.getHours()) + ":" + String(timeClient.getMinutes())+ ":" + String(timeClient.getSeconds()) + "\n";
  message+= "BG: " + String(bg.r) + "-" + String(bg.g) + "-" + String(bg.b) +"\n";
  message+= "SEC: " + String(seconds.r) + "-" + String(seconds.g) + "-" + String(seconds.b) +"\n";
  message+= "MINUTE: " + String(minutes.r) + "-" + String(minutes.g) + "-" + String(minutes.b) +"\n";
  message+= "HOUR: " + String(hours.r) + "-" + String(hours.g) + "-" + String(hours.b) +"\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += request->params();
  message += "\n";
  for (uint8_t i=0; i<request->params(); i++){
    AsyncWebParameter* p = request->getParam(i);
    message += " " + p->name() + ": " + p->value() + "\n";
  }
  request->send(404, "text/plain", message);
}

void send_clock_configuration_html(AsyncWebServerRequest *request)
{
  if (request->args() > 0 )  // Save Settings
  {
    String temp = "";
    if(request->hasParam("light_high")){
      AsyncWebParameter* p = request->getParam("light_high");
      config.light_high = p->value().toInt();
          EEPROM.write(13, config.light_high);  
          EEPROM.commit();
    }
    if(request->hasParam("light_low")){
      AsyncWebParameter* p = request->getParam("light_low");
      config.light_low = p->value().toInt();
          EEPROM.write(12, config.light_low);  
          EEPROM.commit();
    }
    if(request->hasParam("switch_off")){
      AsyncWebParameter* p = request->getParam("switch_off");
      config.switch_off = p->value().toInt();
          EEPROM.write(16, config.switch_off);  
          EEPROM.commit();
    }
    if(request->hasParam("switch_on")){
      AsyncWebParameter* p = request->getParam("switch_on");
      config.switch_on = p->value().toInt();
          EEPROM.write(17, config.switch_on);  
          EEPROM.commit();
    }
    if(request->hasParam("rain")){
      AsyncWebParameter* p = request->getParam("rain");
      config.rain = p->value().toInt();
          EEPROM.write(14, config.rain);  
          EEPROM.commit();
    }
  }
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/clock.html.gz", "text/html");
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}


void send_clock_configuration_values_html(AsyncWebServerRequest *request)
{
	
  String values ="";
  values += "light_high|" + String(config.light_high) + "|input\n";
  values += "light_low|" +  String(config.light_low) + "|input\n";
  values += "switch_off|" +  String(config.switch_off) + "|input\n";
  values += "switch_on|" +  String(config.switch_on) + "|input\n";
  values += "rain|" +  String(config.rain) + "|input\n";
  request->send ( 200, "text/plain", values);
}

void send_color_configuration_values_html(AsyncWebServerRequest *request)
{
	
  long HexRGB; 
  String values ="";
  HexRGB = ((long)hours.r << 16) | ((long)hours.g << 8 ) | (long)hours.b;
  values += "hours|" + (HexRGB == 0 ? "000000": String(HexRGB, HEX)) + "|input\n";
  HexRGB = ((long)lines.r << 16) | ((long)lines.g << 8 ) | (long)lines.b;
  values += "lines|" +  (HexRGB == 0 ? "000000": String(HexRGB, HEX)) + "|input\n";
  values += "p" + String(config.gCurrentPaletteNumber) + "|true|chk\n";
  HexRGB = ((long)minutes.r << 16) | ((long)minutes.g << 8 ) | (long)minutes.b;
  values += "minutes|" + (HexRGB == 0 ? "000000": String(HexRGB, HEX)) + "|input\n";
  request->send ( 200, "text/plain", values);
}


void send_color_configuration_html(AsyncWebServerRequest *request)
{
  if (request->args() > 0 )  // Save Settings
  {
    if(request->hasParam("hours")){
      AsyncWebParameter* p = request->getParam("hours");
      hours = strtol(p->value().c_str(), NULL, 16);
      EEPROM.write(6,hours.r);                     
      EEPROM.write(7,hours.g);
      EEPROM.write(8,hours.b); 
      EEPROM.commit();
    }
    if(request->hasParam("minutes")){
      AsyncWebParameter* p = request->getParam("minutes");
      minutes = strtol(p->value().c_str(), NULL, 16);
      EEPROM.write(3,minutes.r);                     
      EEPROM.write(4,minutes.g);
      EEPROM.write(5,minutes.b); 
      EEPROM.commit();
    }
    if(request->hasParam("seconds")){
      AsyncWebParameter* p = request->getParam("seconds");
      seconds = strtol(p->value().c_str(), NULL, 16);
      EEPROM.write(0,seconds.r);                     
      EEPROM.write(1,seconds.g);
      EEPROM.write(2,seconds.b); 
      EEPROM.commit();
      DEBUG_PRINT("Sec: ");
      DEBUG_PRINT(p->value().c_str());
    }
    if(request->hasParam("lines")){
      AsyncWebParameter* p = request->getParam("lines");
      lines = strtol(p->value().c_str(), NULL, 16);
      EEPROM.write(18,lines.r);                     
      EEPROM.write(19,lines.g);
      EEPROM.write(20,lines.b); 
      EEPROM.commit();
    }
    if(request->hasParam("pattern")){
      AsyncWebParameter* p = request->getParam("pattern");
      config.gCurrentPaletteNumber = p->value().toInt();
      EEPROM.write(15,config.gCurrentPaletteNumber);  
      gCurrentPalette = gGradientPalettes[config.gCurrentPaletteNumber];                   
      EEPROM.commit();
    }
  }
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/color.html.gz", "text/html");
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}


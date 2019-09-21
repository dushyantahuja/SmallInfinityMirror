#include "Arduino.h"
#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Updater.h>

//#include <ESP8266WebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
//#include <PubSubClient.h>
//#include <ESP8266HTTPUpdateServer.h>

AsyncWebServer httpServer(80);
DNSServer dns;
//ESP8266HTTPUpdateServer httpUpdater;

#include <NTPClient.h>
#include <ArduinoJson.h>
#include <IPGeolocation.h>
String IPGeoKey = "b294be4d4a3044d9a39ccf42a564592b";

#define FASTLED_INTERNAL
//#define FASTLED_ESP8266_RAW_PIN_ORDER
#define FASTLED_ESP8266_D1_PIN_ORDER
//#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
#include "EEPROM.h"

#include "palette.h"
#include "config.h"

#include "Page_Admin.h"


//#define MQTT_MAX_PACKET_SIZE 256
//void callback(const MQTT::Publish& pub);


// NTP Servers:

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 360000); //19800

void setup() {
    // put your setup code here, to run once:
    delay(3000);
    Serial.begin(9600);
    FastLED.addLeds<WS2812B, 4, GRB>(leds, 60).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(20);
    fill_rainbow(leds, NUM_LEDS,4);
    //fill_solid(hourleds, 12, bg);
    FastLED.show();
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
    //reverseLEDs();
    Serial.println("Wifi Setup Initiated");
    WiFi.setAutoConnect(true);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    AsyncWiFiManager wifiManager(&httpServer,&dns);
    //WiFiManager wifiManager;
    //wifiManager.resetSettings();
    wifiManager.setTimeout(180);
    if(!wifiManager.autoConnect("smallinfinityClock")) {
      delay(3000);
      ESP.reset();
      delay(5000);
      }
    Serial.println("Wifi Setup Completed");
    MDNS.begin("smallinfinityclock");


    //httpUpdater.setup(&httpServer);
    //httpServer.on("/time", handleRoot);
    // Admin page
    httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request ->send_P(200,"text/html", PAGE_AdminMainPage ); 
    });
    httpServer.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request ->send(SPIFFS, "/www/style.css" ); 
        //request->send(SPIFFS,"/www/style.css","text/css");
    });
    httpServer.on("/microajax.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request ->send(SPIFFS,"/www/microajax.js");
    });
    httpServer.on("/jscolor.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request ->send(SPIFFS,"/www/jscolor.js");
    });
    httpServer.on("/clock.html", HTTP_GET, send_clock_configuration_html);
    httpServer.on("/color.html", HTTP_GET, send_color_configuration_html);
    httpServer.on("/admin/clockconfig", HTTP_GET, send_clock_configuration_values_html);
    //httpServer.on("/admin/colorconfig", HTTP_GET, send_color_configuration_values_html);
    httpServer.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){handleUpdate(request);});
    httpServer.on("/doUpdate", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,
                  size_t len, bool final) {handleDoUpdate(request, filename, index, data, len, final);}
    );
    //httpServer.serveStatic("/", SPIFFS, "/www/");
    /*
    httpServer.on ( "/admin/colorconfig", send_color_configuration_values_html );*/
    httpServer.onNotFound(handleNotFound);
    httpServer.begin();

    IPGeolocation IPG(IPGeoKey);
    IPG.updateStatus();
    timeClient.setTimeOffset(IPG.getOffset()*3600);

    //MDNS.addService("http", "tcp", 80);
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
    //httpServer.handleClient();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
    yield();
}

void showTime(int hr, int mn, int sec) {
  if(sec==0) fill_solid(leds, NUM_LEDS, bg);
  if(( mn % config.rain == 0 && sec == 0)){
       effects();
    }
  colorwaves( leds, mn, gCurrentPalette);
  leds[mn]= minutes;
  leds[hr%12*5]=hours;
  leds[hr%12*5+1]=hours;
  if(hr%12*5-1 > 0)
    leds[hr%12*5-1]=hours;
  else leds[59]=hours; for(byte i = 0; i<60; i+=5){
    leds[i]= lines;  // CRGB(20,30,0); //CRGB(64,64,50);
  }
  if(hr < config.switch_on || hr >= config.switch_off)
    LEDS.setBrightness(constrain(0,config.light_low,100)); // Set brightness to light_low during night - cools down LEDs and power supplies.
  else
    LEDS.setBrightness(constrain(config.light_high,10,255));
}

/*void callback(const MQTT::Publish& pub) {
 Serial.print(pub.topic());
 Serial.print(" => ");
 Serial.println(pub.payload_string());

 String payload = pub.payload_string();
  if(String(pub.topic()) == "smallinfinity/brightness"){
    //int c1 = payload.indexOf(',');
    int h = payload.toInt();
    //int s = payload.substring(c1+1).toInt();
    set_light(h,l);
  }

  if(String(pub.topic()) == "smallinfinity/hour"){
    int c1 = payload.indexOf(',');
    int c2 = payload.indexOf(',',c1+1);
    int h = map(payload.toInt(),0,360,0,255);
    int s = map(payload.substring(c1+1,c2).toInt(),0,100,0,255);
    int v = map(payload.substring(c2+1).toInt(),0,100,0,255);
    set_hour_hsv(h,s,v);
 }
 if(String(pub.topic()) == "smallinfinity/minute"){
    gCurrentPaletteNumber++;
    if (gCurrentPaletteNumber>=gGradientPaletteCount) gCurrentPaletteNumber=0;
    gCurrentPalette = gGradientPalettes[gCurrentPaletteNumber];
    //int c1 = payload.indexOf(',');
    //int c2 = payload.indexOf(',',c1+1);
    //int h = map(payload.toInt(),0,360,0,255);
    //int s = map(payload.substring(c1+1,c2).toInt(),0,100,0,255);
    //int v = map(payload.substring(c2+1).toInt(),0,100,0,255);
    //set_minute_hsv(h,s,v);
 }
 if(String(pub.topic()) == "smallinfinity/second"){
    int c1 = payload.indexOf(',');
    int c2 = payload.indexOf(',',c1+1);
    int h = map(payload.toInt(),0,360,0,255);
    int s = map(payload.substring(c1+1,c2).toInt(),0,100,0,255);
    int v = map(payload.substring(c2+1).toInt(),0,100,0,255);
    set_second_hsv(h,s,v);
 }
 if(String(pub.topic()) == "smallinfinity/bg"){
    int c1 = payload.indexOf(',');
    int c2 = payload.indexOf(',',c1+1);
    int h = map(payload.toInt(),0,360,0,255);
    int s = map(payload.substring(c1+1,c2).toInt(),0,100,0,255);
    int v = map(payload.substring(c2+1).toInt(),0,100,0,255);
    set_bg_hsv(h,s,v);
 }
 if(String(pub.topic()) == "smallinfinity/effects"){
    effects();
 }
 if(String(pub.topic()) == "smallinfinity/clockstatus"){
    clockstatus();
 }
 if(String(pub.topic()) == "smallinfinity/reset"){
   client.publish("smallinfinity/status","Restarting");
   ESP.reset();
 }
}
*/

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


/*void set_hour_hsv(int h, int s, int v){
  CHSV temp;
  temp.h = h;
  temp.s = s;
  temp.v = v;
  hsv2rgb_rainbow(temp,hours);
  EEPROM.write(6,hours.r);
  EEPROM.write(7,hours.g);
  EEPROM.write(8,hours.b);
  EEPROM.commit();
  client.publish("smallinfinity/status","HOUR COLOUR SET");
}

void set_minute_hsv(int h, int s, int v){
  CHSV temp;
  temp.h = h;
  temp.s = s;
  temp.v = v;
  hsv2rgb_rainbow(temp,minutes);
  EEPROM.write(3,minutes.r);
  EEPROM.write(4,minutes.g);
  EEPROM.write(5,minutes.b);
  EEPROM.commit();
  client.publish("smallinfinity/status","MINUTE COLOUR SET");
}

void set_second_hsv(int h, int s, int v){
  CHSV temp;
  temp.h = h;
  temp.s = s;
  temp.v = v;
  hsv2rgb_rainbow(temp,seconds);
  EEPROM.write(0,seconds.r);
  EEPROM.write(1,seconds.g);
  EEPROM.write(2,seconds.b);
  EEPROM.commit();
  client.publish("smallinfinity/status","SECOND COLOUR SET");
}

void set_bg_hsv(int h, int s, int v){
  CHSV temp;
  temp.h = h;
  temp.s = s;
  temp.v = v;
  hsv2rgb_rainbow(temp,bg);
  EEPROM.write(9,bg.r);
  EEPROM.write(10,bg.g);
  EEPROM.write(11,bg.b);
  EEPROM.commit();
  client.publish("smallinfinity/status","BG COLOUR SET");
  fill_solid(leds, NUM_LEDS, bg);
}

void clockstatus(){
  String message = "Status: \n";
  message+= "BG: " + String(bg.r) + "-" + String(bg.g) + "-" + String(bg.b) +"\n";
  message+= "SEC: " + String(seconds.r) + "-" + String(seconds.g) + "-" + String(seconds.b) +"\n";
  message+= "MINUTE: " + String(minutes.r) + "-" + String(minutes.g) + "-" + String(minutes.b) +"\n";
  message+= "HOUR: " + String(hours.r) + "-" + String(hours.g) + "-" + String(hours.b) +"\n";
  message+= "Time: " + String(hour()) + ":" + String(minute())+ ":" + String(second())+"\n";
  client.publish("smallinfinity/status",message);
}


void set_light(int low, int high){
  light_low = low;
  light_high = high;
  EEPROM.write(12,light_low);
  EEPROM.write(13,light_high);
  client.publish("smallinfinity/status","LIGHT SET");
}
*/
// FastLED colorwaves

void colorwaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette) 
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  //uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < numleds; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette( palette, index, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (numleds-1) - pixelnumber;
    
    nblend( ledarray[pixelnumber], newcolor, 128);
  }
}

/*boolean reconnect() {
    if (client.connect("smallinfinity")) {
      // Once connected, publish an announcement...
      client.publish("smallinfinity/status", "smallinfinity Mirror Alive - topics smallinfinity/brighness,smallinfinity/hour,smallinfinity/minute,smallinfinity/second,smallinfinity/bg,smallinfinity/effects subscribed");
      client.publish("smallinfinity/status",ESP.getResetReason());
      // ... and resubscribe
      client.subscribe("smallinfinity/hour");
      client.subscribe("smallinfinity/minute");
      client.subscribe("smallinfinity/second");
      client.subscribe("smallinfinity/bg");
      client.subscribe("smallinfinity/effects");
      client.subscribe("smallinfinity/brightness");
      client.subscribe("smallinfinity/clockstatus");
      client.subscribe("smallinfinity/reset");
    }
  return client.connected();
}*/


void handleNotFound(AsyncWebServerRequest *request){
  //digitalWrite(led, 1);
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
  //digitalWrite(led, 0);
}


/*void reverseLeds() {
  //uint8_t left = 0;
  //uint8_t right = NUM_LEDS-1;
  while (left < right) {
    CRGB temp = leds[left];
    leds[left++] = leds[right];
    leds[right--] = temp;
  }
  for(int i=0; i< NUM_LEDS;i++)
    led2[i]=leds[i];
}*/


void send_clock_configuration_html(AsyncWebServerRequest *request)
{
  if (request->args() > 0 )  // Save Settings
  {
    //int args = request->args();
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
  request ->send(SPIFFS, "/www/clock.html" ); 
  //Serial.println(__FUNCTION__); 
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
  //Serial.println(__FUNCTION__); 
}


String send_color_configuration_values_html(const String& var)
{
  long HexRGB; 
  String values ="";
  HexRGB = ((long)hours.r << 16) | ((long)hours.g << 8 ) | (long)hours.b;
  if(var == "__hours__") return (HexRGB == 0 ? "000000": String(HexRGB, HEX));
  //Serial.println(HexRGB, HEX);
  HexRGB = ((long)minutes.r << 16) | ((long)minutes.g << 8 ) | (long)minutes.b;
  if(var == "__minutes__") return (HexRGB == 0 ? "000000": String(HexRGB, HEX));
  //Serial.println(HexRGB, HEX);
  HexRGB = ((long)seconds.r << 16) | ((long)seconds.g << 8 ) | (long)seconds.b;
  if(var == "__seconds__") return (HexRGB == 0 ? "000000": String(HexRGB, HEX));
  //Serial.println(HexRGB, HEX);
  HexRGB = ((long)lines.r << 16) | ((long)lines.g << 8 ) | (long)lines.b;
  if(var == "__lines__") return (HexRGB == 0 ? "000000": String(HexRGB, HEX));
  //Serial.println(HexRGB, HEX);
  //Serial.println(__FUNCTION__); 
  return String();
}

void send_color_configuration_html(AsyncWebServerRequest *request)
{
  if (request->args() > 0 )  // Save Settings
  {
    //int args = request->args();
    String temp = "";
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
      //Serial.print("Sec: ");
      //Serial.println(p->value().c_str());
    }
    if(request->hasParam("lines")){
      AsyncWebParameter* p = request->getParam("lines");
      lines = strtol(p->value().c_str(), NULL, 16);
      EEPROM.write(18,lines.r);                     
      EEPROM.write(19,lines.g);
      EEPROM.write(20,lines.b); 
      EEPROM.commit();
    }
  }
  request->send(SPIFFS, "/www/color.html", String(), false, send_color_configuration_values_html);
  //Serial.println(__FUNCTION__); 
}


#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
//#include <TimeLib.h>
#include <WiFiUdp.h>
//#include <PubSubClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
//#include <FS.h>

#define FASTLED_INTERNAL
//#define FASTLED_ESP8266_RAW_PIN_ORDER
#define FASTLED_ESP8266_D1_PIN_ORDER
//#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
#include "EEPROM.h"

#include "config.h"

#define NUM_LEDS 60
#define DATA_PIN D2
#define UPDATES_PER_SECOND 35
//#define MQTT_MAX_PACKET_SIZE 256


#include "palette.h"
const TProgmemRGBGradientPalettePtr gGradientPalettes[] = {
  es_emerald_dragon_08_gp,
  Magenta_Evening_gp,
  blues_gp,
  nsa_gp
};
// Count of how many cpt-city gradients are defined:
const uint8_t gGradientPaletteCount =
  sizeof( gGradientPalettes) / sizeof( TProgmemRGBGradientPalettePtr );
// Current palette number from the 'playlist' of color palettes
uint8_t gCurrentPaletteNumber = 1;

CRGBPalette16 gCurrentPalette( gGradientPalettes[gCurrentPaletteNumber]);

CRGBArray<NUM_LEDS> leds; //,led2;
//CRGBArray<12> hourleds;
CRGB minutes,hours,seconds,l,bg,lines;
int light_low, light_high;
boolean missed=0, ledState = 1,  multieffects = 0; //lastsec=1,
byte  rain; //lastsecond,
String message;
//void callback(const MQTT::Publish& pub);


// NTP Servers:

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 360000); //19800

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

void setup() {
    // put your setup code here, to run once:
    delay(3000);
    Serial.begin(74880);
    FastLED.addLeds<WS2812B, 4, GRB>(leds, 60); //.setCorrection(TypicalLEDStrip);
    //FastLED.addLeds<WS2812B, 5, GRB>(hourleds, 12); //.setCorrection(TypicalLEDStrip);
    fill_palette(leds, NUM_LEDS, 0, 0, gCurrentPalette, 10, NOBLEND);
    //fill_palette(hourleds, 12, 0, 0, gCurrentPalette, 10, NOBLEND);
    FastLED.setBrightness(constrain(light_high,10,255));
    //reverseLEDs();
    FastLED.show();
    Serial.println("Wifi Setup Initiated");
    //delay(1000);
    WiFi.setAutoConnect ( true );
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFiManager wifiManager;
    //wifiManager.resetSettings();
    //wifiManager.setConfigPortalTimeout(180);
    wifiManager.setTimeout(180);
    //wifiManager.setConnectTimeout(120);
    if(!wifiManager.autoConnect("smallinfinityClock")) {
      delay(3000);
      ESP.reset();
      delay(5000);
      }
    Serial.println("Wifi Setup Completed");
    MDNS.begin("smallinfinityclock");
    httpUpdater.setup(&httpServer);
    //httpServer.on("/time", handleRoot);
    httpServer.onNotFound(handleNotFound);
    httpServer.begin();

    //MDNS.addService("http", "tcp", 80);
    timeClient.begin();

    EEPROM.begin(512);

    if (EEPROM.read(109) != 2){               // Check if colours have been set or not

      seconds.r = 0;
      seconds.g = 0;
      seconds.b = 0;
      minutes.r = 10;
      minutes.g = 44;
      minutes.b = 53;
      hours.r = 210;
      hours.g = 45;
      hours.b = 0;
      bg.r = 0;
      bg.g = 0;
      bg.b = 0;
      light_low = 0;
      light_high = 65;
      rain = 30;
      gCurrentPaletteNumber = 2;

      EEPROM.write(0,0);                   // Seconds Colour
      EEPROM.write(1,0);
      EEPROM.write(2,0);
      EEPROM.write(3,10);                   // Minutes Colour
      EEPROM.write(4,44);
      EEPROM.write(5,53);
      EEPROM.write(6,210);                     // Hours Colour
      EEPROM.write(7,45);
      EEPROM.write(8,0);
      EEPROM.write(9,0);                     // BG Colour
      EEPROM.write(10,0);
      EEPROM.write(11,0);
      EEPROM.write(12, 0);                   // Light sensitivity - low
      EEPROM.write(13, 65);                  // Light sensitivity - high
      EEPROM.write(14, 30);                  // Minutes for each rainbow
      EEPROM.write(15, 2);                    // Current Palette
      EEPROM.write(109,2);
      EEPROM.commit();
    }
    // Else read the parameters from the EEPROM
    else {
      seconds.r = EEPROM.read(0);
      seconds.g = EEPROM.read(1);
      seconds.b = EEPROM.read(2);
      minutes.r = EEPROM.read(3);
      minutes.g = EEPROM.read(4);
      minutes.b = EEPROM.read(5);
      hours.r = EEPROM.read(6);
      hours.g = EEPROM.read(7);
      hours.b = EEPROM.read(8);
      bg.r = EEPROM.read(9);
      bg.g = EEPROM.read(10);
      bg.b = EEPROM.read(11);
      light_low = EEPROM.read(12);
      light_high = EEPROM.read(13);
      rain = EEPROM.read(14);
      gCurrentPaletteNumber = EEPROM.read(15);
    }
    fill_solid(leds, NUM_LEDS, bg);
    //fill_solid(hourleds, 12, bg);
    FastLED.show();
    gCurrentPalette = gGradientPalettes[gCurrentPaletteNumber];
}

void loop() {
    timeClient.update();
    showTime(timeClient.getHours(),timeClient.getMinutes(),timeClient.getSeconds());
    FastLED.show();
    httpServer.handleClient();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
    yield();
}

void showTime(int hr, int mn, int sec) {
  if(sec==0) fill_solid(leds, NUM_LEDS, bg);
  if(( mn % rain == 0 && sec == 0)){
       effects();
    }
  colorwaves( leds, mn, gCurrentPalette);
  leds[mn]= minutes;
  leds[hr%12*5]=hours;
  leds[hr%12*5+1]=hours;
  if(hr%12*5-1 > 0)
    leds[hr%12*5-1]=hours;
  else leds[59]=hours; for(byte i = 0; i<60; i+=5){
    leds[i]= CRGB(20,30,0); //CRGB(64,64,50);
  }
  if(hr < 7 || hr >= 22)
    LEDS.setBrightness(constrain(0,0,100)); // Set brightness to light_low during night - cools down LEDs and power supplies.
  else
    LEDS.setBrightness(constrain(light_high,10,255));
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


void handleNotFound(){
  //digitalWrite(led, 1);
  message= "Time: ";
  message+= String(timeClient.getHours()) + ":" + String(timeClient.getMinutes())+ ":" + String(timeClient.getSeconds()) + "\n";
  message+= "BG: " + String(bg.r) + "-" + String(bg.g) + "-" + String(bg.b) +"\n";
  message+= "SEC: " + String(seconds.r) + "-" + String(seconds.g) + "-" + String(seconds.b) +"\n";
  message+= "MINUTE: " + String(minutes.r) + "-" + String(minutes.g) + "-" + String(minutes.b) +"\n";
  message+= "HOUR: " + String(hours.r) + "-" + String(hours.g) + "-" + String(hours.b) +"\n";
  /* message += "URI: ";
  message += httpServer.uri();
  message += "\nMethod: ";
  message += (httpServer.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += httpServer.args();
  message += "\n";
  for (uint8_t i=0; i<httpServer.args(); i++){
    message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
  }*/
  httpServer.send(404, "text/plain", message);
  //digitalWrite(led, 0);
}

// ----------------- Code to create a failsafe update

/*void updateFailSafe()
{
  SPIFFS.begin();
  Dir dir = SPIFFS.openDir("/");
  File file = SPIFFS.open("/blinkESP.bin", "r");

  uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  if (!Update.begin(maxSketchSpace, U_FLASH)) { //start with max available size
    Update.printError(Serial);
    Serial.println("ERROR");
  }

  while (file.available()) {
    uint8_t ibuffer[128];
    file.read((uint8_t *)ibuffer, 128);
    Serial.println((char *)ibuffer);
    Update.write(ibuffer, sizeof(ibuffer));
  }

  Serial.print(Update.end(true));
  file.close();
  delay(5000);
  ESP.restart();
}*/


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

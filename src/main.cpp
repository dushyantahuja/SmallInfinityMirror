#include "Arduino.h"
#include <FS.h>
#include <LittleFS.h>
#define SPIFFS LittleFS
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include <ESPAsyncWiFiManager.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <SPIFFSEditor.h>

AsyncWebServer httpServer(80);
DNSServer dns;

//#define DEBUG

#include <ezTime.h>

Timezone myTZ;

//#include <NTPClient.h>
#include <ArduinoJson.h>
//#include <IPGeolocation.h>
//String IPGeoKey = "2a7b4f6d9ff14fd895eef23cc48da063";
//#include "SimpleWeather.h"

#if __cplusplus > 199711L 
    #define register
#endif

#define FASTLED_INTERNAL
#define FASTLED_ESP8266_RAW_PIN_ORDER
//#define FASTLED_ESP8266_D1_PIN_ORDER
//#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"

char ESPNAME[255];
const int DATA_PIN = 4;
#include "palette.h"
#include "config.h"

#include "Page_Admin.h"

// NTP Servers:

WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "time.google.com", 19800, 4320000); //19800 0.asia.pool.ntp.org

void setup()
{
  // put your setup code here, to run once:
  delay(1000);
  //EEPROM.begin(512);
  Serial.begin(74880);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, 60).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(0);
  fill_solid(leds, NUM_LEDS, config.bg);
  //fill_rainbow(leds, NUM_LEDS,4);
  FastLED.show();
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  File file = SPIFFS.open("/deviceid.txt", "r");
  if (!file)
  {
    Serial.println("file open failed");
  }
  else
  {
    int l = file.readBytesUntil('\n', ESPNAME, sizeof(ESPNAME));
    ESPNAME[l] = 0;
    Serial.println(ESPNAME);
  }
  /*file = SPIFFS.open("/pin.txt", "r");
  if (!file)
  {
    Serial.println("file open failed");
  }
  else
  {
    char TEMP_STRING[255];
    int l = file.readBytesUntil('\n', TEMP_STRING, sizeof(TEMP_STRING));
    TEMP_STRING[l] = 0;
    Serial.println(TEMP_STRING);
    sscanf(TEMP_STRING, "%d", &DATA_PIN);
  }*/

  Serial.println("Wifi Setup Initiated");
  WiFi.setAutoConnect(true);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  AsyncWiFiManager wifiManager(&httpServer, &dns);
  //wifiManager.resetSettings();
  wifiManager.setTimeout(180);
  if (!wifiManager.autoConnect(ESPNAME))
  {
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  Serial.println("Wifi Setup Completed");

  // Admin page
  httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", PAGE_AdminMainPage);
  });
  httpServer.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/style.css.gz", "text/css");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer.on("/microajax.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/microajax.js.gz", "text/plain");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer.on("/jscolor.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/jscolor.js.gz", "text/plain");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer.on("/", HTTP_POST, send_clock_configuration_html);
  httpServer.on("/color.html", HTTP_GET, send_color_configuration_html);
  httpServer.on("/admin/clockconfig", HTTP_GET, send_clock_configuration_values_html);
  httpServer.on("/admin/colorconfig", send_color_configuration_values_html);
  httpServer.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "rebooting\n");
    delay(1000);
    ESP.restart();
  });
  httpServer.on("/factory", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "reseting wifi settings\n");
    AsyncWiFiManager wifiManager(&httpServer, &dns);
    wifiManager.resetSettings();
    LittleFS.remove("config.json");
    ESP.restart();
  });
  httpServer.on("/autoupdate", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "<head><meta http-equiv=\"refresh\" content=\"120;url=/\"></head><body>Checking for updates - the clock will restart automatically\n</body>");
    //response->addHeader("Server","ESP Async Web Server");
    autoupdate = true;
    request->send(response);
  });
  httpServer.on("/checktime", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "<head><meta http-equiv=\"refresh\" content=\"20;url=/\"></head><body>Checking time - refreshing clock...\n</body>");
    updateNTP();
    saveDefaults();
    request->send(response);
  });
  AsyncElegantOTA.begin(&httpServer);
  httpServer.addHandler(new SPIFFSEditor("admin", "admin"));
  httpServer.onNotFound(handleNotFound);
  httpServer.begin();

  MDNS.begin(ESPNAME);
  MDNS.addService("http", "tcp", 80);
  
  File configfile = LittleFS.open("config.json", "r");
  if (!configfile) 
    initialiseDefaults();
  // Hard coded defaults in case there is no file
  else
    loadDefaults();

  setServer(config.ntpServerName);
  Serial.println("Getting Time:");
  Serial.println(config.ntpServerName);
  /*while(!waitForSync(15)){
    events();
    Serial.println(myTZ.dateTime("l ~t~h~e jS ~o~f F Y, g:i A"));
    //Serial.print(".");
  }
  while(!waitForSync(15)){
    events();
    Serial.println(myTZ.dateTime("l ~t~h~e jS ~o~f F Y, g:i A"));
    //Serial.print(".");
  }*/
  waitForSync();
  myTZ.setLocation(F("Asia/Kolkata"));
  myTZ.setDefault();
  //setInterval(0);
  Serial.println(lastNtpUpdateTime());

  Serial.println("NTP Done");
  fill_solid(leds, NUM_LEDS, config.bg);
  FastLED.show();
  gCurrentPalette = gGradientPalettes[config.gCurrentPaletteNumber];
  sendIP();
  wdt_enable(WDTO_8S);
  Serial.println("Setup Done");
}

void loop()
{
  //events();
  if (autoupdate)
  {
    checkForUpdates();
    autoupdate = false;
  }
  EVERY_N_MILLISECONDS(1000/UPDATES_PER_SECOND) { 
    showTime(myTZ.hour(), myTZ.minute(), myTZ.second());
    FastLED.show();
  }
  if (myTZ.hour() == 2 && myTZ.minute() == 0 && myTZ.second() == 0)
  {
    ESP.restart();
  }
  MDNS.update();
  yield();
}

void showTime(int hr, int mn, int sec)
{
  if (mn == 0)
    fill_solid(leds, NUM_LEDS, config.bg);
  if ((mn % config.rain == 0 && sec == 0))
  {
    effects();
  }
  colorwaves(leds, mn, gCurrentPalette);
  leds[hr % 12 * 5] = config.hours;
  leds[hr % 12 * 5 + 1] = config.hours;
  if (hr % 12 * 5 - 1 > 0)
    leds[hr % 12 * 5 - 1] = config.hours;
  else
    leds[59] = config.hours;
  for (byte i = 0; i < 60; i += 5)
  {
    leds[i] = config.lines;
  }
  leds[mn] = config.minutes;
  if (hr < config.switch_on || hr >= config.switch_off)
    LEDS.setBrightness(constrain(0, config.light_low, 50)); // Set brightness to light_low during night - cools down LEDs and power supplies.
  else
    LEDS.setBrightness(constrain(config.light_high, 10, 255));
}

void effects()
{
  for (int j = 0; j < 300; j++)
  {
    fadeToBlackBy(leds, NUM_LEDS, 20);
    byte dothue = 0;
    for (int i = 0; i < 8; i++)
    {
      leds[beatsin16(i + 7, 0, NUM_LEDS)] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
    yield();
  }
  fill_solid(leds, NUM_LEDS, config.bg);
}

void handleNotFound(AsyncWebServerRequest *request)
{
  String message;
  message = "Time: ";
  //message += String(timeClient.getHours()) + ":" + String(timeClient.getMinutes()) + ":" + String(timeClient.getSeconds()) + "\n";
  message += myTZ.dateTime("l ~t~h~e jS ~o~f F Y, g:i A");
  message += "BG: " + String(config.bg.r) + "-" + String(config.bg.g) + "-" + String(config.bg.b) + "\n";
  message += "SEC: " + String(config.seconds.r) + "-" + String(config.seconds.g) + "-" + String(config.seconds.b) + "\n";
  message += "MINUTE: " + String(config.minutes.r) + "-" + String(config.minutes.g) + "-" + String(config.minutes.b) + "\n";
  message += "HOUR: " + String(config.hours.r) + "-" + String(config.hours.g) + "-" + String(config.hours.b) + "\n";
  message += "Version: " + String(FW_VERSION) + "\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += request->params();
  message += "\n";
  for (uint8_t i = 0; i < request->params(); i++)
  {
    AsyncWebParameter *p = request->getParam(i);
    message += " " + p->name() + ": " + p->value() + "\n";
  }
  request->send(404, "text/plain", message);
}

void send_clock_configuration_html(AsyncWebServerRequest *request)
{
  if (request->args() > 0) // Save Settings
  {
    String temp = "";
    if (request->hasParam("light_high",true))
    {
      AsyncWebParameter *p = request->getParam("light_high",true);
      config.light_high = p->value().toInt();
      saveDefaults();
    }
    if (request->hasParam("light_low",true))
    {
      AsyncWebParameter *p = request->getParam("light_low",true);
      config.light_low = p->value().toInt();
      saveDefaults();
    }
    if (request->hasParam("switch_off",true))
    {
      AsyncWebParameter *p = request->getParam("switch_off",true);
      config.switch_off = p->value().toInt();
      saveDefaults();
    }
    if (request->hasParam("switch_on",true))
    {
      AsyncWebParameter *p = request->getParam("switch_on",true);
      config.switch_on = p->value().toInt();
      saveDefaults();
    }
    if (request->hasParam("rain",true))
    {
      AsyncWebParameter *p = request->getParam("rain",true);
      config.rain = p->value().toInt();
      saveDefaults();
    }
  }
  request->redirect("/");
}

void send_clock_configuration_values_html(AsyncWebServerRequest *request)
{

  String values = "";
  values += "light_high|" + String(config.light_high) + "|input\n";
  values += "light_low|" + String(config.light_low) + "|input\n";
  values += "switch_off|" + String(config.switch_off) + "|input\n";
  values += "switch_on|" + String(config.switch_on) + "|input\n";
  values += "rain|" + String(config.rain) + "|input\n";
  values += "currenttime|"+myTZ.dateTime("l ~t~h~e jS ~o~f F Y, g:i A")+"|input";
  request->send(200, "text/plain", values);
}

void send_color_configuration_values_html(AsyncWebServerRequest *request)
{

  long HexRGB;
  String values = "";
  HexRGB = ((long)config.hours.r << 16) | ((long)config.hours.g << 8) | (long)config.hours.b;
  values += "hours|" + (HexRGB == 0 ? "000000" : String(HexRGB, HEX)) + "|input\n";
  HexRGB = ((long)config.lines.r << 16) | ((long)config.lines.g << 8) | (long)config.lines.b;
  values += "lines|" + (HexRGB == 0 ? "000000" : String(HexRGB, HEX)) + "|input\n";
  values += "p" + String(config.gCurrentPaletteNumber) + "|true|chk\n";
  HexRGB = ((long)config.minutes.r << 16) | ((long)config.minutes.g << 8) | (long)config.minutes.b;
  values += "minutes|" + (HexRGB == 0 ? "000000" : String(HexRGB, HEX)) + "|input\n";

  request->send(200, "text/plain", values);
}

void send_color_configuration_html(AsyncWebServerRequest *request)
{
  if (request->args() > 0) // Save Settings
  {
    if (request->hasParam("hours"))
    {
      AsyncWebParameter *p = request->getParam("hours");
      config.hours = strtol(p->value().c_str(), NULL, 16);
      saveDefaults();
    }
    if (request->hasParam("minutes"))
    {
      AsyncWebParameter *p = request->getParam("minutes");
      config.minutes = strtol(p->value().c_str(), NULL, 16);
      saveDefaults();
    }
    if (request->hasParam("seconds"))
    {
      AsyncWebParameter *p = request->getParam("seconds");
      config.seconds = strtol(p->value().c_str(), NULL, 16);
      saveDefaults();
      DEBUG_PRINT("Sec: ");
      DEBUG_PRINT(p->value().c_str());
    }
    if (request->hasParam("lines"))
    {
      AsyncWebParameter *p = request->getParam("lines");
      config.lines = strtol(p->value().c_str(), NULL, 16);
      saveDefaults();
    }
    if (request->hasParam("pattern"))
    {
      AsyncWebParameter *p = request->getParam("pattern");
      config.gCurrentPaletteNumber = p->value().toInt();
      gCurrentPalette = gGradientPalettes[config.gCurrentPaletteNumber];
      saveDefaults();
    }
  }
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/color.html.gz", "text/html");
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

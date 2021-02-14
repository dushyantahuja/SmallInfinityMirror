#include "Arduino.h"
#include <FS.h>
#include <LittleFS.h>
#define SPIFFS LittleFS
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
//#include <Updater.h>

#include <ESPAsyncWiFiManager.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <SPIFFSEditor.h>

AsyncWebServer httpServer(80);
DNSServer dns;

#define DEBUG

#include <NTPClient.h>
#include <ArduinoJson.h>
//#include <IPGeolocation.h>
//String IPGeoKey = "2a7b4f6d9ff14fd895eef23cc48da063";
//#include "SimpleWeather.h"

#define FASTLED_INTERNAL
#define FASTLED_ESP8266_RAW_PIN_ORDER
//#define FASTLED_ESP8266_D1_PIN_ORDER
#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
#include "EEPROM.h"

char ESPNAME[255];
int DATA_PIN = 4;
#include "palette.h"
#include "config.h"

#include "Page_Admin.h"

// NTP Servers:

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "192.168.78.2", 19800, 43200000); //19800 0.asia.pool.ntp.org

void setup()
{
  // put your setup code here, to run once:
  //delay(1000);
  Serial.begin(9600);
  FastLED.addLeds<WS2812B, 4, GRB>(leds, 60).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(0);
  fill_solid(leds, NUM_LEDS, bg);
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
  file = SPIFFS.open("/pin.txt", "r");
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
  }
  Serial.println("Wifi Setup Initiated");
  AsyncWiFiManager wifiManager(&httpServer, &dns);
  //wifiManager.resetSettings();
  WiFi.setAutoConnect(true);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
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
    EEPROM.write(109,22);
    EEPROM.commit();
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
    //response->addHeader("Server","ESP Async Web Server");
    //IPGeolocation IPG(IPGeoKey);
    //IPGeo I;
    //IPG.updateStatus(&I);
    //config.timezoneoffset = (int)(I.offset * 3600);
    timeClient.forceUpdate();
    saveDefaults();
    request->send(response);
  });
  AsyncElegantOTA.begin(&httpServer);
  /*httpServer.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) { handleUpdate(request); });
  httpServer.on(
      "/doUpdate", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data,
         size_t len, bool final) { handleDoUpdate(request, filename, index, data, len, final); });*/
  httpServer.addHandler(new SPIFFSEditor("admin", "admin"));
  httpServer.onNotFound(handleNotFound);
  httpServer.begin();

  

  MDNS.begin(ESPNAME);
  MDNS.addService("http", "tcp", 80);
  
  EEPROM.begin(512);
  if (EEPROM.read(109) != 6)
    saveDefaults();
  // Else read the parameters from the EEPROM
  else
    loadDefaults();
  //timeClient.setTimeOffset(config.timezoneoffset);
  timeClient.begin();
  timeClient.update();
  fill_solid(leds, NUM_LEDS, bg);
  FastLED.show();
  gCurrentPalette = gGradientPalettes[config.gCurrentPaletteNumber];
  sendIP();
  wdt_enable(WDTO_8S);
}

void loop()
{
  AsyncElegantOTA.loop();
  if (autoupdate)
  {
    checkForUpdates();
    autoupdate = false;
  }
  showTime(timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
  FastLED.show();
  if (timeClient.getHours() == 2 && timeClient.getMinutes() == 0 && timeClient.getSeconds() == 0)
  {
    //checkForUpdates();
    //IPGeolocation IPG(IPGeoKey);
    //IPGeo I;
    //IPG.updateStatus(&I);
    //config.timezoneoffset = (int)(I.offset * 3600);
    //timeClient.setTimeOffset(config.timezoneoffset);
    //saveDefaults();
    timeClient.forceUpdate();
    ESP.restart();
  }
  MDNS.update();
  FastLED.delay(1000 / UPDATES_PER_SECOND);
  yield();
}

void showTime(int hr, int mn, int sec)
{
  if (mn == 0)
    fill_solid(leds, NUM_LEDS, bg);
  if ((mn % config.rain == 0 && sec == 0))
  {
    effects();
  }
  colorwaves(leds, mn, gCurrentPalette);
  leds[hr % 12 * 5] = hours;
  leds[hr % 12 * 5 + 1] = hours;
  if (hr % 12 * 5 - 1 > 0)
    leds[hr % 12 * 5 - 1] = hours;
  else
    leds[59] = hours;
  for (byte i = 0; i < 60; i += 5)
  {
    leds[i] = lines;
  }
  leds[mn] = minutes;
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
  }
  fill_solid(leds, NUM_LEDS, bg);
}

void handleNotFound(AsyncWebServerRequest *request)
{
  String message;
  message = "Time: ";
  message += String(timeClient.getHours()) + ":" + String(timeClient.getMinutes()) + ":" + String(timeClient.getSeconds()) + "\n";
  message += "BG: " + String(bg.r) + "-" + String(bg.g) + "-" + String(bg.b) + "\n";
  message += "SEC: " + String(seconds.r) + "-" + String(seconds.g) + "-" + String(seconds.b) + "\n";
  message += "MINUTE: " + String(minutes.r) + "-" + String(minutes.g) + "-" + String(minutes.b) + "\n";
  message += "HOUR: " + String(hours.r) + "-" + String(hours.g) + "-" + String(hours.b) + "\n";
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
      EEPROM.write(13, config.light_high);
      EEPROM.commit();
    }
    if (request->hasParam("light_low",true))
    {
      AsyncWebParameter *p = request->getParam("light_low",true);
      config.light_low = p->value().toInt();
      EEPROM.write(12, config.light_low);
      EEPROM.commit();
    }
    if (request->hasParam("switch_off",true))
    {
      AsyncWebParameter *p = request->getParam("switch_off",true);
      config.switch_off = p->value().toInt();
      EEPROM.write(16, config.switch_off);
      EEPROM.commit();
    }
    if (request->hasParam("switch_on",true))
    {
      AsyncWebParameter *p = request->getParam("switch_on",true);
      config.switch_on = p->value().toInt();
      EEPROM.write(17, config.switch_on);
      EEPROM.commit();
    }
    if (request->hasParam("rain",true))
    {
      AsyncWebParameter *p = request->getParam("rain",true);
      config.rain = p->value().toInt();
      EEPROM.write(14, config.rain);
      EEPROM.commit();
    }
  }
  //AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/clock.html.gz", "text/html");
  //response->addHeader("Content-Encoding", "gzip");
  //request->send(response);
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
  request->send(200, "text/plain", values);
}

void send_color_configuration_values_html(AsyncWebServerRequest *request)
{

  long HexRGB;
  String values = "";
  HexRGB = ((long)hours.r << 16) | ((long)hours.g << 8) | (long)hours.b;
  values += "hours|" + (HexRGB == 0 ? "000000" : String(HexRGB, HEX)) + "|input\n";
  HexRGB = ((long)lines.r << 16) | ((long)lines.g << 8) | (long)lines.b;
  values += "lines|" + (HexRGB == 0 ? "000000" : String(HexRGB, HEX)) + "|input\n";
  values += "p" + String(config.gCurrentPaletteNumber) + "|true|chk\n";
  HexRGB = ((long)minutes.r << 16) | ((long)minutes.g << 8) | (long)minutes.b;
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
      hours = strtol(p->value().c_str(), NULL, 16);
      EEPROM.write(6, hours.r);
      EEPROM.write(7, hours.g);
      EEPROM.write(8, hours.b);
      EEPROM.commit();
    }
    if (request->hasParam("minutes"))
    {
      AsyncWebParameter *p = request->getParam("minutes");
      minutes = strtol(p->value().c_str(), NULL, 16);
      EEPROM.write(3, minutes.r);
      EEPROM.write(4, minutes.g);
      EEPROM.write(5, minutes.b);
      EEPROM.commit();
    }
    if (request->hasParam("seconds"))
    {
      AsyncWebParameter *p = request->getParam("seconds");
      seconds = strtol(p->value().c_str(), NULL, 16);
      EEPROM.write(0, seconds.r);
      EEPROM.write(1, seconds.g);
      EEPROM.write(2, seconds.b);
      EEPROM.commit();
      DEBUG_PRINT("Sec: ");
      DEBUG_PRINT(p->value().c_str());
    }
    if (request->hasParam("lines"))
    {
      AsyncWebParameter *p = request->getParam("lines");
      lines = strtol(p->value().c_str(), NULL, 16);
      EEPROM.write(18, lines.r);
      EEPROM.write(19, lines.g);
      EEPROM.write(20, lines.b);
      EEPROM.commit();
    }
    if (request->hasParam("pattern"))
    {
      AsyncWebParameter *p = request->getParam("pattern");
      config.gCurrentPaletteNumber = p->value().toInt();
      EEPROM.write(15, config.gCurrentPaletteNumber);
      gCurrentPalette = gGradientPalettes[config.gCurrentPaletteNumber];
      EEPROM.commit();
    }
  }
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/color.html.gz", "text/html");
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

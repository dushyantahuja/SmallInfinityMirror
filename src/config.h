#include <FS.h>
#include <ArduinoJson.h>

// Function Definitions

void handleNotFound();
void colorwaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette);
void effects();
void showTime(int hr, int mn, int sec);


struct strConfig {
    CRGB seconds;
    CRGB minutes;
    CRGB hours;
    CRGB bg;
    int light_low;
    int light_high;
    int rain;
    byte gCurrentPaletteNumber;
	String ntpServerName;
	long Update_Time_Via_NTP_Every;
	long timezoneoffset;
    boolean autoTimezone;
	boolean daylight;
}   config;

/*
bool  saveConfig ( )  { 
  StaticJsonBuffer < 200 >  jsonBuffer ; 
  JsonObject &  json  =  jsonBuffer . createObject ( ) ; 
  json [ "serverName" ]  =  "api.example.com" ; 
  json [ "accessToken" ]  =  "128du9as8du12eoue8da98h123ueh9h98" ;

  File  configFile  =  SPIFFS . open ( "/config.json" ,  "w" ) ; 
  if  ( ! configFile )  { 
    Serial . println ( "Failed to open config file for writing" ) ; 
    return  false ; 
  }

  json . printTo ( configFile ) ; 
  return  true ; 
}

bool  loadConfig ( )  { 
  File  configFile  =  SPIFFS . open ( "/config.json" ,  "r" ) ; 
  if  ( ! configFile )  { 
    Serial . println ( "Failed to open config file" ) ; 
    return  false ; 
  }

  size_t  size  =  configFile . size ( ) ; 
  if  ( size  >  1024 )  { 
    Serial . println ( "Config file size is too large" ) ; 
    return  false ; 
  }

  // Allocate a buffer to store contents of the file. 
  std : : unique_ptr < char [ ] >  buf ( new  char [ size ] ) ;

  // We don't use String here because ArduinoJson library requires the input 
  // buffer to be mutable. If you don't use ArduinoJson, you may as well 
  // use configFile.readString instead. 
  configFile . readBytes ( buf . get ( ) ,  size ) ;

  StaticJsonBuffer < 200 >  jsonBuffer ; 
  JsonObject &  json  =  jsonBuffer . parseObject ( buf . get ( ) ) ;

  if  ( ! json . success ( ) )  { 
    Serial . println ( "Failed to parse config file" ) ; 
    return  false ; 
  }

  const  char *  serverName  =  json [ "serverName" ] ; 
  const  char *  accessToken  =  json [ "accessToken" ] ;

  // Real world application would store these values ​​in some variables for 
  // later use.

  Serial . print ( "Loaded serverName:" ) ; 
  Serial . println ( serverName ) ; 
  Serial . print ( "Loaded accessToken:" ) ; 
  Serial . println ( accessToken ) ; 
  return  true ; 
}
*/
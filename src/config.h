#define NUM_LEDS 60
#define DATA_PIN D2
#define UPDATES_PER_SECOND 35

// Function Definitions

void handleNotFound();
void colorwaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette);
void effects();
void showTime(int hr, int mn, int sec);


const TProgmemRGBGradientPalettePtr gGradientPalettes[] = {
  es_emerald_dragon_08_gp,
  Magenta_Evening_gp,
  plain_blue,
  blues_gp,
  nsa_gp
};
// Count of how many cpt-city gradients are defined:
const uint8_t gGradientPaletteCount =
  sizeof( gGradientPalettes) / sizeof( TProgmemRGBGradientPalettePtr );
// Current palette number from the 'playlist' of color palettes
uint8_t gCurrentPaletteNumber = 1;

CRGBPalette16 gCurrentPalette( gGradientPalettes[gCurrentPaletteNumber]);

CRGBArray<NUM_LEDS> leds;
CRGB minutes,hours,seconds,bg,lines;
//int light_low, light_high;
boolean missed=0, ledState = 1,  multieffects = 0;
//byte  rain;
String message;


struct strConfig {
    /*CRGB seconds;
    CRGB minutes;
    CRGB hours;
    CRGB bg;
    CRGB lines;*/
    int light_low;
    int light_high;
    int rain;
    int gCurrentPaletteNumber;
	  //String ntpServerName;
	  int Update_Time_Via_NTP_Every;
	  int timezoneoffset;
    int autoTimezone;
	  int daylight;
    int switch_off;
    int switch_on;
}   config;


bool saveDefaults(){
  {               // Check if colours have been set or not

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
      lines.r = 64;
      lines.g = 64;
      lines.b = 50;
      config.light_low = 0;
      config.light_high = 65;
      config.rain = 30;
      config.gCurrentPaletteNumber = 2;
      config.switch_off = 22;
      config.switch_on = 7;


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
      EEPROM.write(16,22);                    // Switch Off
      EEPROM.write(17,7);                     // Switch On
      EEPROM.write(18,64);                     //lines colour
      EEPROM.write(19,64);
      EEPROM.write(20,50);
      EEPROM.write(109,4);
      EEPROM.commit();
    }
    return true;
}

bool loadDefaults()
{
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
      lines.r = EEPROM.read(18);
      lines.g = EEPROM.read(19);
      lines.b = EEPROM.read(20);
      config.light_low = EEPROM.read(12);
      config.light_high = EEPROM.read(13);
      config.rain = EEPROM.read(14);
      config.gCurrentPaletteNumber = EEPROM.read(15);
      config.switch_off = EEPROM.read(16);
      config.switch_on = EEPROM.read(17);
      return true;
    }

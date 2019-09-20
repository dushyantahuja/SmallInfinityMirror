//
//  HTML PAGE
//
const char Page_ColorConfiguration[] PROGMEM = R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<script src="jscolor.js"></script>
<a href="/"  class="btn btn--s"><</a>&nbsp;&nbsp;<strong>Clock Configuration</strong>
<hr>
Configuration:<br>
<form action="/clock.html" method="get">
<table border="0"  cellspacing="0" cellpadding="3" style="width:500px" >
<tr><td align="right">Daytime Brightness (0-255):</td><td><input type="number" id="light_high" name="light_high" value="" min="0" max="255"></td></tr>
<tr><td align="right">Nighttime Brightness (0-255):</td><td><input type="number" id="light_low" name="light_low" value="" min="0" max="255"></td></tr>
<tr><td align="right">Time to Switch On (0-12):</td><td><input type="number" id="switch_on" name="switch_on" value="" min="0" max="12"></td></tr>
<tr><td align="right">Time to Switch Off (13-23):</td><td><input type="number" id="switch_off" name="switch_off" value="" min="13" max="23"></td></tr>
<tr><td align="right">Effects every n minutes:</td><td><input type="number" id="rain" name="rain" value="" min="0" max="59"></td></tr>
<tr><td colspan="2" align="center"><input type="submit" style="width:150px" class="btn btn--m btn--blue" value="Save"></td></tr>
</table>
</form>
<hr>
</table>
<script>
  

window.onload = function ()
{
  load("style.css","css", function() 
  {
    load("microajax.js","js", function() 
    {
        setValues("/admin/clockconfig");
    });
  });
}
function load(e,t,n){if("js"==t){var a=document.createElement("script");a.src=e,a.type="text/javascript",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}else if("css"==t){var a=document.createElement("link");a.href=e,a.rel="stylesheet",a.type="text/css",a.async=!1,a.onload=function(){n()},document.getElementsByTagName("head")[0].appendChild(a)}}



</script>

)=====";


void send_color_configuration_html()
{
  if (httpServer.args() > 0 )  // Save Settings
  {
    String temp = "";
    for ( uint8_t i = 0; i < httpServer.args(); i++ ) {
      if (httpServer.argName(i) == "light_high") {
          config.light_high = httpServer.arg(i).toInt();
          EEPROM.write(13, config.light_high);  
          EEPROM.commit();
      }
      if (httpServer.argName(i) == "light_low") {
        config.light_low =  httpServer.arg(i).toInt(); 
        EEPROM.write(13, config.light_high);  
          EEPROM.commit();
      }
      if (httpServer.argName(i) == "switch_off") {
          config.switch_off = httpServer.arg(i).toInt();
          EEPROM.write(16, config.switch_off);  
          EEPROM.commit();
      }
      if (httpServer.argName(i) == "switch_on") {
          config.switch_on = httpServer.arg(i).toInt();
          EEPROM.write(17, config.switch_on);  
          EEPROM.commit();
      }
      if (httpServer.argName(i) == "rain") {
          config.rain = httpServer.arg(i).toInt();
          EEPROM.write(14, config.rain);  
          EEPROM.commit();
      }
    }
  }
  httpServer.send ( 200, "text/html", FPSTR(PAGE_ClockConfiguration) ); 
  //Serial.println(__FUNCTION__); 
}

void send_color_configuration_values_html()
{
	
  String values ="";
  values += "light_high|" + String(config.light_high) + "|input\n";
  values += "light_low|" +  String(config.light_low) + "|input\n";
  values += "switch_off|" +  String(config.switch_off) + "|input\n";
  values += "switch_on|" +  String(config.switch_on) + "|input\n";
  values += "rain|" +  String(config.rain) + "|input\n";
  httpServer.send ( 200, "text/plain", values);
  //Serial.println(__FUNCTION__); 
}

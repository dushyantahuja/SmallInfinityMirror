//
//  HTML PAGE
//
const char Page_ColorConfiguration[] PROGMEM = R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<script src="/jscolor.js"></script>
<a href="/"  class="btn btn--s"><</a>&nbsp;&nbsp;<strong>Choose Colors</strong>
<hr>
Configuration:<br>
<form action="/color.html" method="get">
<table border="0"  cellspacing="0" cellpadding="3" style="width:500px" >
<tr><td align="right">Hour</td><td><input name="hours" type="jscolor" class="jscolor" value=""></td></tr>
<tr><td align="right">Minute</td><td><input name="minutes" type="jscolor" class="jscolor" value=""></td></tr>
<tr><td align="right">Seconds</td><td><input name="seconds" type="jscolor" class="jscolor" value=""></td></tr>
<tr><td align="right">5 Min Markers</td><td><input name="lines" type="jscolor" class="jscolor" value=""></td></tr>
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
        setValues("/admin/colorconfig");
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
      if (httpServer.argName(i) == "hours") {
          hours = strtol(httpServer.arg(i).c_str(), NULL, 16);
          EEPROM.write(6,hours.r);                     
          EEPROM.write(7,hours.g);
          EEPROM.write(8,hours.b); 
          EEPROM.commit();
      }
      if (httpServer.argName(i) == "minutes") {
          minutes = strtol(httpServer.arg(i).c_str(), NULL, 16);
          EEPROM.write(3,minutes.r);                     
          EEPROM.write(4,minutes.g);
          EEPROM.write(5,minutes.b); 
          EEPROM.commit();
      }
      if (httpServer.argName(i) == "seconds") {
          seconds = strtol(httpServer.arg(i).c_str(), NULL, 16);
          EEPROM.write(0,seconds.r);                     
          EEPROM.write(1,seconds.g);
          EEPROM.write(2,seconds.b); 
          EEPROM.commit();
      }
      if (httpServer.argName(i) == "lines") {
          lines = strtol(httpServer.arg(i).c_str(), NULL, 16);
          EEPROM.write(18,lines.r);                     
          EEPROM.write(19,lines.g);
          EEPROM.write(20,lines.b); 
          EEPROM.commit();
      }
    }
  }
  httpServer.send ( 200, "text/html", FPSTR(Page_ColorConfiguration) ); 
  //Serial.println(__FUNCTION__); 
}

void send_color_configuration_values_html()
{
  long HexRGB; 
  String values ="";
  HexRGB = ((long)hours.r << 16) | ((long)hours.g << 8 ) | (long)hours.b;
  values += "hours|" + String(HexRGB, HEX) + "|input\n";
  HexRGB = ((long)minutes.r << 16) | ((long)minutes.g << 8 ) | (long)minutes.b;
  values += "minutes|" +  String(HexRGB, HEX) + "|input\n";
  HexRGB = ((long)seconds.r << 16) | ((long)seconds.g << 8 ) | (long)seconds.b;
  values += "seconds|" +  String(HexRGB, HEX) + "|input\n";
  HexRGB = ((long)lines.r << 16) | ((long)lines.g << 8 ) | (long)lines.b;
  values += "lines|" +  String(HexRGB, HEX) + "|input\n";
  //values += "rain|" +  String(HexRGB, HEX)) + "|input\n";
  httpServer.send ( 200, "text/plain", values);
  //Serial.println(__FUNCTION__); 
}

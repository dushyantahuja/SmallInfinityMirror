//
//  HTML PAGE
//

const char PAGE_AdminMainPage[] PROGMEM = R"=====(
<meta name="viewport" content="width=device-width, initial-scale=1" />
<strong>Administration</strong>
<hr>
Configuration:<br>
<form action="/" method="post">
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
<a href="color.html" style="width:250px" class="btn btn--m btn--blue" >Choose Colors</a><br>
<!--<a href="pattern.html" style="width:250px" class="btn btn--m btn--blue" >Choose Pattern</a><br>-->
<a href="update" style="width:250px" class="btn btn--m btn--blue" >Upload New Firmware</a><br>
<a href="autoupdate" style="width:250px" class="btn btn--m btn--blue" >Check for Update</a><br>
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
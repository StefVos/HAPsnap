# HapSnap
esp32

This is my first ESP32 project. It is a Apple Homekit compatible switch with a touch sensor button and a wifi configuration manager. 

Connect a touch sensor (or just a wire) to pin D4. 

Scan the wifi connect QR code (left image) with the standard iPhone camera app, or search for a new wifi network named "esp32" and connect with password "esp32pwd". When you connect to this network a login page will automatically be shown, this is called a captive portal and has been tested on an iPhone. On this login page you find a list of all the wifi networks the ESP32 can receive (only 2.4 GHz). When you click on your network a popup for entering your wifi password will show.

<div style="display:flex; flex-direction:row;">
  <img src="https://github.com/StefVos/HapSnap/blob/master/wifi.png" height="180px" style="padding-bottom: 4px;"/>
  <img src="https://github.com/StefVos/HapSnap/blob/master/qrcode007.png" height="250px" />
</div>

After the ESP32 is connected to your wifi you can add the switch to your home in homekit. Scan the QR pairing code (image on the right) or manualy enter "053-58-197". Now you can control the led on the ESP32board with your phone and with the touch sensor. 

4 existing projects where combined to create this.
1) Homekit magic https://github.com/younghyunjo/esp32-homekit.git
2) Wifi Manager https://github.com/tonyp7/esp32-wifi-manager
3) Fake DNS Server for "captive portal" functionality https://github.com/cornelis-61/esp32_Captdns
4) Pairing using QR-code <a href="https://github.com/maximkulkin/esp-homekit" >https://github.com/maximkulkin/esp-homekit</a>

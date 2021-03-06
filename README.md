# Wi-Fi 2.4Ghz Jammer
<p align="center">Running Wi-Fi Jammer on ANKER powerbank.</p>

<p align="center"><img width="65%" alt="esp8266 deauther with smartphone" src="https://raw.githubusercontent.com/Aayush-Security/Wi-Fi-Jammer-Arduino-ESP/master/images/CA15AFA7-4F0A-40C4-B634-FDAA2ABFA219.jpeg"></p>
<p align="center">ESP8266 SSID:"pwned" web interface on 192.168.4.1.</p>
<p align="center"><img width="65%" alt="Interface" src="https://raw.githubusercontent.com/Aayush-Security/Wi-Fi-Jammer-Arduino-ESP/master/images/Screenshot%20from%202018-02-25%2017-17-41.png"></p>

<p align="center">
<a href="https://instagram.com/aayush_iisr"> <img width="9%" height="9%" src="http://pngimg.com/uploads/instagram/instagram_PNG12.png"></a>
</p>
<p align="center">
<a href="https://www.linkedin.com/in/aayush-trivedi-b14162136/"> <img width="9%" height="9%" src="https://vignette.wikia.nocookie.net/the-most-popular-girls-in-school/images/1/1a/Linkedin.png/revision/latest?cb=20150730204727"></a>
</p>


### Method-1 Easy Installation Direct upload binary(.bin) via Linux Terminal

You will need [either Python 2.7 or Python 3.4 or newer](https://www.python.org/downloads/) installed on your system.

The latest stable esptool.py release can be installed from [pypi](http://pypi.python.org/pypi/esptool) via pip:

```
 pip install esptool
```

With some Python installations this may not work and you'll receive an error, try `python -m pip install esptool` or `pip2 install esptool`.

After installing, you will have `esptool.py` installed into the default Python executables directory and you should be able to run it with the command `esptool.py`.

In Linux, the current user may not have access to serial ports and a "Permission Denied" error will appear. On most Linux distributions, the solution is to add the user to the `dialout` group with a command like`sudo chmod -R 777 /dev/ttyUSB0`.

### NOW IT's TIME TO UPLOAD BIN FILE TO YOUR ESP8266 CHIP.
**1** First erase flash on ESP8266 for error free .bin uploading.

`esptool.py erase_flash`

**2** Upload bin file.

`esptool.py write_flash 0x0000 (directry of bin file or just simply drag n drop on terminal)`

DONE!!!
Enjoy

### Method-2 Uploading Bin file using Nodemce-flash (Windows only)--very easy
Refer this link https://github.com/nodemcu/nodemcu-flasher
### Method-3 Compiling and uploading sketch .ino with Arduino IDE (Windows and Linux)
Again on linux if IDE shows error type this on terminal `sudo chmod -R 777 /dev/ttyUSB0
`. 

**0** Download the source code of this project.

**1** Install [Arduino](https://www.arduino.cc/en/Main/Software) and open it.

**2** Go to `File` > `Preferences`

**3** Add `http://arduino.esp8266.com/stable/package_esp8266com_index.json` to the Additional Boards Manager URLs. (source: https://github.com/esp8266/Arduino)

**4** Go to `Tools` > `Board` > `Boards Manager`

**5** Type in `esp8266`

**6** Select version `2.0.0` and click on `Install` (**must be version 2.0.0!**)

![screenshot of arduino, selecting the right version](https://github.com/Aayush-Security/Wi-Fi-Jammer-Arduino-ESP/blob/master/images/2B4227DA-4E01-4B54-B70E-38B3F224D9D0.jpeg?raw=truep)

**7** Go to `File` > `Preferences`

**8** Open the folder path under `More preferences can be edited directly in the file`

![screenshot of arduino, opening folder path](https://raw.githubusercontent.com/Aayush-Security/Wi-Fi-Jammer-Arduino-ESP/master/images/FB593583-BD0D-45E8-89E4-692031D85996.jpeg)

**9** Go to `packages` > `esp8266` > `hardware` > `esp8266` > `2.0.0` > `tools` > `sdk` > `include`

**10** Open `user_interface.h` with a text editor

**11** Scroll down and before `#endif` add following lines:

```
typedef void (*freedom_outside_cb_t)(uint8 status);
int wifi_register_send_pkt_freedom_cb(freedom_outside_cb_t cb);
void wifi_unregister_send_pkt_freedom_cb(void);
int wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
```  

![screenshot of notepad, copy paste the right code](https://raw.githubusercontent.com/Aayush-Security/Wi-Fi-Jammer-Arduino-ESP/master/images/FE935EF5-FCD4-4FB5-B5B0-CA83BF95BAAB.jpeg)

**don't forget to save!**  

**12** Go to the fix folder of this project

**13** Copy ESP8266Wi-Fi.cpp and ESP8266Wi-Fi.h

**14** Paste these files here `packages` > `esp8266` > `hardware` > `esp8266` > `2.0.0` > `libraries` > `ESP8266WiFi` > `src`

**15** Open `esp8266_deauther` > `esp8266_deauther.ino` in Arduino

**16** Select your ESP8266 board at `Tools` > `Board` and the right port at `Tools` > `Port`  
If no port shows up you may have to reinstall the drivers.

**17** Depending on your board you may have to adjust the `Tools` > `Board` > `Flash Frequency` and the `Tools` > `Board` > `Flash Size`. I use a `160MHz` flash frequency and a `4M (3M SPIFFS)` flash size.

**18** Upload!
### After sucessful upload your ESP8266 broadcast SSID named "pwned" password is "deauther".It can be operated on 192.168.4.1
 

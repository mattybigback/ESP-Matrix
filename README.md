# ESP Matrix Display
A simple scrolling message display that can be updated over WiFi from a web browser.

## Hardware Requirements

The code has been tested on the ESP-01S, the NodeMCU 1.0 board and the Wemos D1 R2 board, but any ESP8266-based board should work with a little modification to the code. Both hardware and software SPI are supported.

The matrix module used was a generic 32x8 module, commonly sold on Amazon, Ebay etc. The MD_Parola library defines this as FC16 hardware.

An example schematic is included for use with the ESP-01S.

## Libraries

The following  libraries must be installed from the Arduino IDE library manager. The versions below are the versions I used. Updated versions may or may not work, but I make no guarantees.

| Library | Version | Author |
|---|---|---|
| [MD_MAX72XX](https://github.com/MajicDesigns/MD_MAX72XX) | 3.2.1 | majicDesigns |
| [MD_Parola](https://github.com/MajicDesigns/MD_Parola) | 3.3.0 | majicDesigns |
| [WiFiManager](https://github.com/tzapu/WiFiManager) | 0.15.0 | tzapu |

Additonally, the ESP8266 Arduino core must be installed.

## Operation

### Access point mode (first boot)
When the module first starts it will be in WiFiManager mode. Nothing is displayed during this time.

The ESP module starts in access point mode, with a unique network name (SSID). Connect to this network using your device and you should be automatically taken to the configuration page. To access the configuration page manually navigate to the static IP address in any web browser (by default this is set as 10.0.0.1).

Access point mode is entered whenever a successful connection to WiFi could not be made.

### Normal operation

Once a connection to a WiFi network has been established the matrix will scroll the IP address of the configuration page, followed by either the build-in message or whichever message is stored in the ESP file system. The first message to be shown on connection to the WiFi network will always be the local IP addess.

The configuration page is a simple form that has fields for a message, the display intensity (brightness) and scroll speed. When a new message is sent the old message is interrupted and the display cleared. Messages are stored in the ESP module's file system, so they are retained even when the system is powered down.

### Factory Reset

A reset routine has been added to the code which allows the WiFi creentials, stored message and stored settings to be erased. Due to the limited availability of GPIO pins on the ESP-01S module this is shared with the CS line of the MAX7219 module. On boards with more available IO lines it is possible to adjust the code so that a dedicated pin is used. See the provided schematic for the appropriate way to connect the reset button when using the shared pin.

To perform a factory reset press and hold the factory reset button during power up, and release it after a few moments. If the reset was successful the word "RESET" will be displayed, followed by "SETUP". The device is now in access point mode and ready to be connected to a new WiFi network.

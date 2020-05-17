# ESP Matrix Display
A simple scrolling message display that can be updated over WiFi from a web browser

## Hardware Requirements

The code has been tested on a NodeMCU 1.0 board and a Wemos D1 R2 board, but any ESP-12E/F-based board should work with a little modification to the code.

The matrix module used was a generic 32x8 module, commonly sold on Amazon, Ebay etc. The MD_Parola library defines this as FC16 hardware.

## Libraries

The following  libraries must be installed from the Arduino IDE library manager. The versions below are the versions I used. Updated versions may or may not work, but I make no guarantees.

| Library | Version | Author |
|---|---|---|
| MD_MAX72XX | 3.2.1 | majicDesigns |
| MD_Parola | 3.3.0 | majicDesigns |
| WiFiManager | 0.15.0 | tzapu |

Additonally, the ESP8266 Arduino core must be installed.

## Operation

When the module first starts it will be in WiFiManager mode. This creates an access point which allows for an SSID and passcode to be entered. To access the configuration page manually navigate to the static IP address in any web browser (by defult this is set as 10.0.0.1) The display does not scroll during this time.

Once a connection to a WiFi network has been established the matrix will scroll the IP address of the configuration page, followed by either the build-in message or whichever message is stored in the ESP file system. The first message to be shown on connection to the WiFi network will always be the local IP addess.

The configuration page is a simple form that has fields for a message, the display intensity (brightness) and scroll speed. When a new message is sent the old message is interrupted and the display cleared.

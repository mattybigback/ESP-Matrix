/* ESP8266-based WiFi scrolling display by Matt H
   http://m-harrison.org
   https://github.com/mattybigback

   Required hardware:

   ESP12E/ESP12F development board (Tested using NodeMCU 1.0 and Wemos D1R2)
   32x8 MAX7219 matrix module

   MD_MAX72xx and MD_Parola libraries written and maintainesd by Marco Colli (MajicDesigns)
   https://github.com/MajicDesigns
   https://github.com/MajicDesigns/MD_MAX72XX
   https://github.com/MajicDesigns/MD_Parola

   WiFiMagager written and maintained by tzapu
   https://github.com/tzapu/WiFiManager


   Use this code at your own risk. I know I do.
*/

//File system libraries
#include <FS.h>
#include <LittleFS.h> //SPIFFS depreciated, use LittleFS instead

//Network-related libraries
#include <ESP8266WiFi.h>;
#include <ESP8266WebServer.h>;
#include <DNSServer.h>
#include <WiFiManager.h>

//Matrix-related libraries
#include <SPI.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>

//Web pages - raw literals stored within their own header files
#include "index.h"
#include "update.h"

//Set the matrix type to the type commonly found on eBay.
//Read MD_Parola documentation for which option to use for other modules
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

//Set number of MAX72xx chips being used
#define MAX_DEVICES 4

//Define SPI pins. YOU MAY NEED TO CHANGE THESE
#define CLK_PIN   D5 // SCK
#define DATA_PIN  D7 // MOSI
#define CS_PIN    D8 // SS

//Matrix display array
const int BUF_SIZE = 500;           //Be careful here.
                                    //If you increase the buffer over 500 then you must
                                    //also increase the size of mainPageBuffer. 500 is PLENTY.
char curMessage[BUF_SIZE] = { "" };
char newMessage[BUF_SIZE] = { "" };

//New Message Flag
bool newMessageAvailable = true;

//Reset Display Flag
bool resetDisplay = false;

//Matrix display properties used on startup to display IP address
int intensity = 15;
int scrollSpeed = 90;

//WiFi AP Name - Should not exceed 24 chracters as the maximum length for an SSID is 32 characters, and 8 are used for the board ID
const char* APNamePrefix = "Mattrix";

//FS Paths
#define messagePath "/message.txt"
#define intensityConfPath "/intens.txt"
#define speedConfPath "/speed.txt"

//Scrolling effects
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
const int scrollPause = 0; // in milliseconds. Not used by default - holds the screen at the end of the message

//Instantiate objects
ESP8266WebServer server(80); //Server on port 80 (default HTTP port) - can change it to a different port if need be.
MD_Parola matrix = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

//File objects for FS
File messageFile;
File scrollSpeedConfFile;
File intensityConfFile;

void setup() {
  Serial.begin(115200);   //For debugging
  delay(2000); //Stabilises serial port
  Serial.println();
  //Begin matrix, set scroll speed and intensity from global variables
  matrix.begin();
  matrix.setSpeed(scrollSpeed);
  matrix.setIntensity(intensity);

  // Start WiFiManager
  startWifiManager();

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //
  server.begin();
  server.on("/", handleRoot);       //Function to call when root page is loaded
  server.on("/update", handleForm); //Function to call when form is submitted and update page is loaded
  Serial.println("HTTP server started");

  char ipAddress[15]; //Char array to store human readable IP address
  sprintf(ipAddress, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);

  //Copy IP address to newMessage display buffer so that it is scrolled across the display
  strcpy(newMessage, ipAddress);
  //Set up text scroll animation
  matrix.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
  //Set newMessageAvailable and resetDisplay flags
  newMessageAvailable = true;
  resetDisplay = true;
  //Trigger scroll animation
  messageScroll();

  //Begin LittleFS and check if it successfully mounts FS.
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
  }
  String incomingFS; //String object for passing data to and from LittleFS

  //Check if message file exists, and if it doesn't then create one
  if (!LittleFS.exists(messagePath)) {
    Serial.println("No message file exists. Creating one now...");
    messageFile = LittleFS.open(messagePath, "w");
    messageFile.print("Hello, World!");
    messageFile.close();
  }
  //Read message file until it reaches the null terminator character and store contents in the newMessage char array
  messageFile = LittleFS.open(messagePath, "r");
  incomingFS = messageFile.readStringUntil('\n');
  incomingFS.toCharArray(newMessage, BUF_SIZE);

  //Check if speed conf file exists, and if it doesn't then create one
  if (!LittleFS.exists(speedConfPath)) {
    Serial.println("No speed file exists. Creating one now...");
    scrollSpeedConfFile = LittleFS.open(speedConfPath, "w");
    scrollSpeedConfFile.print("50");
    scrollSpeedConfFile.close();
  }
  //Read speed conf file until it reaches the null terminator character, convert it to an int and store it in the scrollSpeed global variable
  scrollSpeedConfFile = LittleFS.open(speedConfPath, "r");
  incomingFS = scrollSpeedConfFile.readStringUntil('\n');
  scrollSpeed = incomingFS.toInt();

  //Check if intensity conf file exists, and if it doesn't then create one
  if (!LittleFS.exists(intensityConfPath)) {
    Serial.println("No intensity file exists. Creating one now...");
    intensityConfFile = LittleFS.open(intensityConfPath, "w");
    intensityConfFile.print("7");
    intensityConfFile.close();
  }
  //Read intensity conf file until it reaches the null terminator character, convert it to an int and store it in the intensity global variable
  intensityConfFile = LittleFS.open(intensityConfPath, "r");
  incomingFS = intensityConfFile.readStringUntil('\n');
  intensity = incomingFS.toInt();

  //Set new message flag
  //By not also setting the displayReset flag the first message can continue to scroll
  newMessageAvailable = true;
}


void loop() {
  // put your main code here, to run repeatedly:
  messageScroll();          //Scroll the message
  server.handleClient();    //Keep web server ticking over
}

void messageScroll() {
  //If the display is still animating OR the resetDisplay flag has been set
  if (matrix.displayAnimate() || resetDisplay) {

    if (newMessageAvailable) {          //If a new message has been set
      resetDisplay = false;             //Clear the resetDisplay flag
      strcpy(curMessage, newMessage);   //Copy the newMessage buffer to curMessage
      newMessageAvailable = false;      //Clear the newMessageAvailabe flag
      matrix.setSpeed(scrollSpeed);     //Set scroll speed from global variable
      matrix.setIntensity(intensity);   //Set intensity from global variable
    }
    matrix.displayReset();              //If display has finished animating reset the animation
  }
}

//Server response to a request for root page
void handleRoot() {
  char mainPageBuffer[1024];
  sprintf(mainPageBuffer, mainPage, BUF_SIZE, curMessage, intensity, scrollSpeed);
  Serial.println(mainPageBuffer); //Useful for debugging
  server.send(200, "text/html", mainPageBuffer);    //Send mainPageBuffer array as HTML
}

//Server response to incoming data from form
void handleForm() {
  String incomingMessage = server.arg("messageToScroll");   //Must use strings as that is what the library returns (BLEURGH)
  String incomingIntensity = server.arg("intensity");       //Just look at all of them
  String incomingscrollSpeed = server.arg("speed");         //All that memory wasted

  incomingMessage.toCharArray(newMessage, BUF_SIZE);             //Convert incoming message to a char array (much better);
  intensity = incomingIntensity.toInt();                    //Convert incoming intensity value to int
  scrollSpeed = incomingscrollSpeed.toInt();                //Comvert incoming scroll value to int

  //Write message, speed and intensity files to LittleFS
  messageFile = LittleFS.open(messagePath, "w");
  messageFile.print(newMessage);
  messageFile.close();
  scrollSpeedConfFile = LittleFS.open(speedConfPath, "w");
  scrollSpeedConfFile.print(scrollSpeed);
  scrollSpeedConfFile.close();
  intensityConfFile = LittleFS.open(intensityConfPath, "w");
  intensityConfFile.print(intensity);
  intensityConfFile.close();

  //Set the newMessageAvailable flag, clear the display, reset the display and set the resetDisplay flag
  newMessageAvailable = true;
  matrix.displayClear();
  matrix.displayReset();
  resetDisplay = true;

  //Debug output
  Serial.println(newMessage);
  Serial.println(intensity);
  Serial.println(scrollSpeed);
  Serial.println();

  //Send mainPage array as HTML
  server.send(200, "text/html", updatePage);
}

void startWifiManager() {
  //Create instance of WiFiManager
  WiFiManager wifiManager;

  //Buffer for AP Name
  char APName[32];
  //Puts the APNamePrefix defined in the setup in the APName buffer and then adds the ESP module ID, formatted as 8 characters of Hex (leading zeros added)
  sprintf(APName, "%S%08X", APNamePrefix, ESP.getChipId());
  //Sets a static IP so that it is easy to connect to while in AP mode
  wifiManager.setAPStaticIPConfig(IPAddress(10, 0, 0, 1), IPAddress(10, 0, 0, 1), IPAddress(255, 0, 0, 0));
  wifiManager.autoConnect(APName);
}

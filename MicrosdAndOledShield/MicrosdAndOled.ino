/**
   The MIT License (MIT)

   Copyright (c) 2018 by ThingPulse, Daniel Eichhorn
   Copyright (c) 2018 by Fabrice Weinberg

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.

   ThingPulse invests considerable time and money to develop these open source libraries.
   Please support us by buying our products (and not the clones) from
   https://thingpulse.com

*/

// Include the correct display library

// For a connection via I2C using the Arduino Wire include:
#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"
#include <WiFi.h>
#include <WebServer.h>        //To make a simple webserver that accomodates only one simultaneous client
#include <ESPmDNS.h>
#ifndef STASSID
#define STASSID "yourssid"
#define STAPSK  "yourwifipass"
#endif
#include "FS.h" 
#include "SPI.h"//To access the file system 
#include "SD.h" 
#define VSPI FSPI
#define SD_miso  9
#define SD_mosi  11
#define SD_sck   7
#define SD_ss    16
#define led 15
const char* ssid = STASSID;
const char* password = STAPSK;
const char* host = "S2Mini";
// OR #include "SH1106Wire.h"   // legacy: #include "SH1106.h"
SPIClass * vspi = NULL;
//HSPI OR VSPI
SPIClass SDSPISD(HSPI);
// For a connection via I2C using brzo_i2c (must be installed) include:
// #include <brzo_i2c.h>        // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Brzo.h"
// OR #include "SH1106Brzo.h"

// For a connection via SPI include:
// #include <SPI.h>             // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Spi.h"
// OR #include "SH1106SPi.h"


// Optionally include custom images
#include "images.h"

// Initialize the OLED display using Arduino Wire:
SSD1306Wire display(0x3c, 33, 35);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h
// SSD1306Wire display(0x3c, D3, D5);  // ADDRESS, SDA, SCL  -  If not, they can be specified manually.
// SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_128_32);  // ADDRESS, SDA, SCL, OLEDDISPLAY_GEOMETRY  -  Extra param required for 128x32 displays.
// SH1106Wire display(0x3c, SDA, SCL);     // ADDRESS, SDA, SCL

// Initialize the OLED display using brzo_i2c:
// SSD1306Brzo display(0x3c, D3, D5);  // ADDRESS, SDA, SCL
// or
// SH1106Brzo display(0x3c, D3, D5);   // ADDRESS, SDA, SCL

// Initialize the OLED display using SPI:
// D5 -> CLK
// D7 -> MOSI (DOUT)
// D0 -> RES
// D2 -> DC
// D8 -> CS
// SSD1306Spi display(D0, D2, D8);  // RES, DC, CS
// or
// SH1106Spi display(D0, D2);       // RES, DC
WebServer server(80); //Set the server at port 80

#define DEMO_DURATION 3000
typedef void (*Demo)(void);

int demoMode = 0;
int counter = 1;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  WiFi.mode(WIFI_STA); //Connect it to a wifi Network
  WiFi.begin(ssid, password); //with the ssid and password defined earlier
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {// Wait for connection
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(host);
    Serial.println(".local");
  }
  server.onNotFound(handleRoot); //Calls the function handleRoot regardless of the server uri ex.(192.168.100.110/edit server uri is "/edit")
  server.begin();//starts the server
  Serial.println("HTTP server started");
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  SDSPISD.begin(SD_sck, SD_miso, SD_mosi, SD_ss);
  if (!SD.begin(SD_ss, SDSPISD,80000000)) { //initialize SD Card
    Serial.println("SD Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();  //get sd card type
  if (cardType == CARD_NONE) { //check if the sd card presence
    Serial.println("No SD Card attached");
    return;
  }
  // Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

}

void handleRoot() {

  /* SD_MMC pertains to the sd card "memory". It is save as a
    variable at the same address given to fs in the fs library
    with "FS" class to enable the file system wrapper to make
    changes on the sd cards memory */
  fs::FS &fs = SD;
  String path = server.uri(); //saves the to a string server uri ex.(192.168.100.110/edit server uri is "/edit")
  Serial.print("path ");  Serial.println(path);

  //To send the index.html when the serves uri is "/"
  if (path.endsWith("/")) {
    path += "index.html";
  }

  //gets the extension name and its corresponding content type
  String contentType = getContentType(path);
  Serial.print("contentType ");
  Serial.println(contentType);
  File file = fs.open(path, "r"); //Open the File with file name = to path with intention to read it. For other modes see <a href="https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html" style="font-size: 13.5px;"> https://arduino-esp8266.readthedocs.io/en/latest/...</a>
  size_t sent = server.streamFile(file, contentType); //sends the file to the server references from <a href="https://github.com/espressif/arduino-esp32/blob/master/libraries/WebServer/src/WebServer.h" style="font-size: 13.5px;"> https://arduino-esp8266.readthedocs.io/en/latest/...</a>
  file.close(); //Close the file
}

//This functions returns a String of content type
String getContentType(String filename) {
  if (server.hasArg("download")) { // check if the parameter "download" exists
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) { //check if the string filename ends with ".htm"
    return "text/html";
  } else if (filename.endsWith(".php")) { //check if the string filename ends with ".htm"
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
//  File dataFile = SD.open(filename.c_str());
//  
//  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
//    Serial.println("Sent less data than expected!");
//  }
//  dataFile.close();
  return "text/plain";
}

void drawFontFaceDemo() {
  // Font Demo1
  // create more fonts at http://oleddisplay.squix.ch/
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Hello world");
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, "Hello world");
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 26, "Hello world");
}

void drawTextFlowDemo() {
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 0, 128,
                             "Lorem ipsum\n dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore." );
}

void drawTextAlignmentDemo() {
  // Text alignment demo
  display.setFont(ArialMT_Plain_10);

  // The coordinates define the left starting point of the text
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 10, "Left aligned (0,10)");

  // The coordinates define the center of the text
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, "Center aligned (64,22)");

  // The coordinates define the right end of the text
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128, 33, "Right aligned (128,33)");
}

void drawRectDemo() {
  // Draw a pixel at given position
  for (int i = 0; i < 10; i++) {
    display.setPixel(i, i);
    display.setPixel(10 - i, i);
  }
  display.drawRect(12, 12, 20, 20);

  // Fill the rectangle
  display.fillRect(14, 14, 17, 17);

  // Draw a line horizontally
  display.drawHorizontalLine(0, 40, 20);

  // Draw a line horizontally
  display.drawVerticalLine(40, 0, 20);
}

void drawCircleDemo() {
  for (int i = 1; i < 8; i++) {
    display.setColor(WHITE);
    display.drawCircle(32, 32, i * 3);
    if (i % 2 == 0) {
      display.setColor(BLACK);
    }
    display.fillCircle(96, 32, 32 - i * 3);
  }
}

void drawProgressBarDemo() {
  int progress = (counter / 5) % 100;
  // draw the progress bar
  display.drawProgressBar(0, 32, 120, 10, progress);

  // draw the percentage as String
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 15, String(progress) + "%");
}

void drawImageDemo() {
  // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
  // on how to create xbm files
  display.drawXbm(34, 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

Demo demos[] = {drawFontFaceDemo, drawTextFlowDemo, drawTextAlignmentDemo, drawRectDemo, drawCircleDemo, drawProgressBarDemo, drawImageDemo};
int demoLength = (sizeof(demos) / sizeof(Demo));
long timeSinceLastModeSwitch = 0;

void loop() {
  
  //eine Pause von 250ms
  
  //deaktivieren der LED
  //eine Pause von 250ms
  // clear the display
  display.clear();
  // draw the current demo method
  demos[demoMode]();

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128, 54, String(millis()));
  // write the buffer to the display
  display.display();

  if (millis() - timeSinceLastModeSwitch > DEMO_DURATION) {
    demoMode = (demoMode + 1)  % demoLength;
    timeSinceLastModeSwitch = millis();
  }
  counter++;
  server.handleClient();
  delay(10);
}

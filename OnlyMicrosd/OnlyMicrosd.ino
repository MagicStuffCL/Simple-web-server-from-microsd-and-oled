//Load necessaries libraries from https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFi.h
#include <WiFi.h>             // Add Wifi Capabilities
#include <WebServer.h>        //To make a simple webserver that accomodates only one simultaneous client
#include <ESPmDNS.h>
#include "FS.h" 
#include "SPI.h"//To access the file system 
#include "SD.h"          //To access the SD Card
//replace with your network credentials
#ifndef STASSID
#define STASSID "yourssid"
#define STAPSK  "yourwifipass"
#endif
#define VSPI FSPI
#define SD_miso  9
#define SD_mosi  11
#define SD_sck   7
#define SD_ss    16
#define led 15

SPIClass * vspi = NULL;
//HSPI OR VSPI
SPIClass SDSPI(HSPI);
const char* ssid = STASSID;
const char* password = STAPSK;
const char* host = "S2Mini";

WebServer server(80); //Set the server at port 80


void setup() {

  Serial.begin(115200);
  Serial.println();
  Serial.println();
  pinMode(led, OUTPUT);

  SDSPI.begin(SD_sck, SD_miso, SD_mosi, SD_ss);
  /////////////////sdcard
  if (!SD.begin(SD_ss, SDSPI,80000000)) { //initialize SD Card
    Serial.println("SD Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();  //get sd card type
  if (cardType == CARD_NONE) { //check if the sd card presence
    Serial.println("No SD Card attached");
    return;
  }

  ////////////////Wifi
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
  Serial.println(WiFi.localIP()); //prints the local ip given to the esp by the router
  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(host);
    Serial.println(".local");
  }
  ////////////////Server
  server.onNotFound(handleRoot); //Calls the function handleRoot regardless of the server uri ex.(192.168.100.110/edit server uri is "/edit")
  server.begin();//starts the server
  Serial.println("HTTP server started");
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

void loop() {
  digitalWrite(led, HIGH);

  /*Waits For connection on the server and is responsible for
    sending and receivingdata request on server uri from the client*/
  server.handleClient();
  delay(2);
}

/*
   Based on WebSocketClient.ino example


*/

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <M5Stack.h>
#include <WebSocketsClient.h>


#include "colors.h"
#include "credentials.h"

// Constants from credentials.h so we don't check them into git!

// //SSID and password of wifi connection
//const char* ssid = "SSID";
//const char* password = "Password";

// //URL and port of socket server
// //not sure how well this deals with DNS, safest to use IP addrs
//const char* ws_server = "192.168.1.199";
//int ws_port = 8765;

// Uses websocket library  by Markus Sattler (Links2004)


// uses msgpack to efficiently xmit/rcv data
#include <ArduinoJson.h>


// blackboard for text to display

#define COLS 32
#define ROWS 10

char disp[ROWS][COLS + 1];
char label[3][COLS+1]; // store button soft labels here
int msg_lines = 0;
// set to 1 if dsiplay text has changed so we don't redraw unless new data
uint8_t redraw = 0;


// fpr LCD

#define GFXFF 1
#define GLCD  0
#define FONT2 2
#define FONT4 4
#define FONT6 6
#define FONT7 7
#define FONT8 8

WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define USE_SERIAL Serial

void hexdump(const void *mem, uint32_t len, uint8_t cols = 16) {
  const uint8_t* src = (const uint8_t*) mem;
  USE_SERIAL.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
  for (uint32_t i = 0; i < len; i++) {
    if (i % cols == 0) {
      USE_SERIAL.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
    }
    USE_SERIAL.printf("%02X ", *src);
    src++;
  }
  USE_SERIAL.printf("\n");
}


void handle_payload(uint8_t *payload) {
  // deserialize and deal with messages in msgpack format

  byte val;
  int i;


  // # deserialize MessagePack packet
  // Inside the brackets, 200 is the capacity of the memory pool in bytes.
  // Don't forget to change this value to match your JSON document.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<300> doc;

  DeserializationError error = deserializeMsgPack(doc, payload);

  // Test if parsing succeeded.
  if (error) {
    USE_SERIAL.print("deserializeMsgPack() failed: ");
    USE_SERIAL.println(error.c_str());
    return;
  }


  if (1) { // handy for debugging what we are getting, don't need it otherwise
    serializeJsonPretty(doc, Serial);
  }

  // if using Python on the other end, you can msgpack encode a dict and send it
  // retrieve data here indexed by dict key. Easy, hunh?


  msg_lines =  doc["msg"].size();
  for (i = 0; i < msg_lines; i++) {
    // copy message lines into display array
    String msg = doc["msg"][i];
    msg.toCharArray(disp[i], COLS);
  }

  
  String msg = doc["time"];
  msg.toCharArray(disp[i], COLS);

  for (i = 0; i < 3; i++) {
      // copy message lines into display array
      String msg = doc["labels"][i];
      if (msg){
        msg.toCharArray(label[i], COLS);
      }
  }
  
  
  redraw = 1;


  //USE_SERIAL.println(msg1);


}



void clear_blackboard() {
  // set all display strings to zero-length
  for (int i = 0; i < ROWS; i++) {
    disp[i][0] = '\0';
  }
}

// handler for websocket events -- mostly recieve
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch (type) {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);

      // send message to server when Connected
      // webSocket.sendTXT("Connected");
      break;
    case WStype_TEXT:
      USE_SERIAL.printf("[WSc] get text:\n");

      // send message to server
      // webSocket.sendTXT("message here");
      break;
    case WStype_BIN:
      //USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
      //hexdump(payload, length);
      handle_payload(payload);

      // send data to server
      // webSocket.sendBIN(payload, length);
      break;
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      USE_SERIAL.printf("[WSc] fragment or error %s\n", payload);
      break;
  }

}

void setup() {
  // USE_SERIAL.begin(921600);

  clear_blackboard();

  USE_SERIAL.begin(115200);

  M5.begin();

  USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  M5.Lcd.setFreeFont(&FreeSans9pt7b);                 // Select the font

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    M5.Lcd.printf("[SETUP] BOOT WAIT %d...\n", t);

    delay(250);
  }

  M5.Lcd.print("\nConnecting");

  WiFi.begin((char *)ssid, (char *)password);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(250);
    Serial.print(".");
    M5.Lcd.print(".");
  }


  // Print our IP address
  Serial.println("Connected!");
  Serial.print("My IP address: ");
  Serial.println(WiFi.localIP());

  M5.Lcd.println("Connected!");
  M5.Lcd.print("My IP address: ");
  M5.Lcd.println(WiFi.localIP());

  delay(1000);
  // server address, port and optional path
  webSocket.begin(ws_server, ws_port);

  // event handler, called when server sends us data
  webSocket.onEvent(webSocketEvent);

  // use HTTP Basic Authorization this is optional remove if not needed
  //webSocket.setAuthorization("user", "Password");

  // try ever 5000 again if connection has failed
  webSocket.setReconnectInterval(5000);

  Serial.println("websocket  Connected?");

}


void paint_display() {
  int i;
  if (redraw == 0)
    return;
  M5.Lcd.setTextDatum(MC_DATUM);

  // Set text colour to orange with black background
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  M5.Lcd.fillScreen(TFT_BLACK);            // Clear screen
  M5.Lcd.setFreeFont(&FreeSans12pt7b);                 // Select the font
  for (i = 0; i < msg_lines; i++) {
    M5.Lcd.drawString((const char*)disp[i], 160, 50+(26*i), GFXFF);// Print the string name o
  }

  // last one will be time
  M5.Lcd.setTextColor(TFT_GRAY, TFT_BLACK);
  M5.Lcd.drawString((const char*)disp[i], 160, 60+(26*i), GFXFF);// Print the string name o


  // now draw labels

  M5.Lcd.setTextColor(TFT_BLUE, TFT_BLACK);
  for (i = 0; i < msg_lines; i++) {
    M5.Lcd.drawString((const char*)label[i], 60 + (95*i), 220, GFXFF);
  }
  
  redraw = 0;

}

void loop() {
  webSocket.loop();
  M5.update();

  paint_display();

  // if you want to use Releasefor("was released for"), use .wasReleasefor(int time) below
  if (M5.BtnA.wasReleased()) {
    webSocket.sendTXT("butA");
  } else if (M5.BtnB.wasReleased()) {
    webSocket.sendTXT("butB");
  } else if (M5.BtnC.wasReleased()) {
    webSocket.sendTXT("butC");
  } else if (M5.BtnA.wasReleasefor(500)) {
    webSocket.sendTXT("butA_long");
  }

}

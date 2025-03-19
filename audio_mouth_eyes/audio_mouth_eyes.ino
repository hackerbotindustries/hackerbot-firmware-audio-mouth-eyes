/****************************************************************************** 
Hackerbot Industries, LLC
Created By: Ian Bernstein
Created:    April 2024
Updated:    2025.03.18

This sketch is written for the "Audio/Mouth/Eye" PCBA and controls the mouth of
hackerbot. It also acts as a command pass through to the eyes.

TODO - Add I2C Slave code so other parts of hackerbot can send commands change
modes for the mouth. Add a mode for raw control of the mouth.

Special thanks to the following for their code contributions to this codebase:
Randy  - https://github.com/rbeiter
*********************************************************************************/

#include <Adafruit_NeoPixel.h>
#include <SerialCmd.h>
#include <Wire.h>
#include "Hackerbot_Shared.h"
#include "SerialCmd_Helper.h"

// Audio Mouth Eyes software version
#define VERSION_NUMBER 4

// Defines and variables for spectrum analyzer
#define STROBE 2
#define RESET 3
#define DC_One A0

int freq_amp;
int Frequencies_One[7];
int i;

// Defines for mouth neopixel
#define PIN_MOUTH 1
#define NUMPIXELS 6 // Popular NeoPixel ring size

// Set up neopixels
Adafruit_NeoPixel pixels(NUMPIXELS, PIN_MOUTH, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel onboard_pixel(1, PIN_NEOPIXEL);

// Timing variables
long previousMillis = 0;
int ledState = LOW;
long blinkInterval = 1000;

// Other defines and variables
byte I2CRxArray[16];
byte I2CTxArray[16];
byte cmd = 0;

// Set up the serial command processor
SerialCmdHelper mySerCmd(Serial);
int8_t ret;


// I2C Rx Handler
void I2C_RxHandler(int numBytes) {
  String query = String();
  char CharArray[32];

  Serial.print("INFO: I2C Byte Received... ");
  for (int i = 0; i < numBytes; i++) {
    I2CRxArray[i] = Wire.read();
    Serial.print("0x");
    Serial.print(I2CRxArray[i], HEX);
    Serial.print(" ");
  }

  Serial.println();

  // Parse incoming commands
  switch (I2CRxArray[0]) {
    case I2C_COMMAND_PING: // Ping
      cmd = I2C_COMMAND_PING;
      I2CTxArray[0] = 0x01;
      break;
    case I2C_COMMAND_VERSION: // Version
      cmd = I2C_COMMAND_VERSION;
      I2CTxArray[0] = VERSION_NUMBER;
      break;
    case I2C_COMMAND_H_GAZE: // Set_GAZE Command - Params(int8_t x, int8_t y) where x and y are >= -100 && <= 100 with 0,0 eyes looking centered straight ahead
      Serial.println("INFO: Set_GAZE command received");

      if (numBytes != 3) {
        Serial.print("INFO: Set_GAZE didn't receive expected byte count - ");
        Serial.print(numBytes);
        Serial.println(" != 3");
        break;
      }
      query = "GAZE," + (String)(float((int8_t)I2CRxArray[1]) / 100.0f) + "," + (String)(float((int8_t)I2CRxArray[2]) / 100.0f);

      // Convert the query string to a char array
      query.toCharArray(CharArray, query.length() + 1);
      Serial.println(CharArray);
      ret = mySerCmd.ReadString(CharArray);
      break;
  }
}

// I2C Tx Handler
void I2C_TxHandler(void) {
  switch (cmd) {
    case I2C_COMMAND_PING: // Ping
      Wire.write(I2CTxArray[0]);
      break;
    case I2C_COMMAND_VERSION: // Version
      Wire.write(I2CTxArray[0]);
      break;
  }
}


// -------------------------------------------------------
// User Functions
// -------------------------------------------------------
void sendOK(void) {
  mySerCmd.Print((char *) "INFO: OK\r\n");
}


// -------------------------------------------------------
// Functions for SerialCmd
// -------------------------------------------------------
void send_PING(void) {
  sendOK();
}

// Reports the current fw version
// Example - "VERSION"
void Get_Version(void) {
  mySerCmd.Print((char *) "INFO: Audio Mouth Eyes Firmware (v");
  mySerCmd.Print(VERSION_NUMBER);
  mySerCmd.Print((char *) ".0)\r\n");

  sendOK();
}


// Sets the gaze of the Hackerbot head's eyes
// Parameters
// float: x (position between -1.0 and 1.0)
// float: y (position between -1.0 and 1.0)
// Example - "GAZE,-0.8,0.2"
void set_GAZE(void) {
  char buf[80] = {0};
  float eyeTargetX = 0.0;
  float eyeTargetY = 0.0;

  if (!mySerCmd.ReadNextFloat(&eyeTargetX) || !mySerCmd.ReadNextFloat(&eyeTargetY)) {
    mySerCmd.Print((char *) "ERROR: Missing parameter\r\n");
    return;
  }

  // Constrain values to acceptable range
  eyeTargetX = constrain(eyeTargetX, -1.0, 1.0);
  eyeTargetY = constrain(eyeTargetY, -1.0, 1.0);

  sprintf(buf, "STATUS: Setting: eyeTargetX: %.2f, eyeTargetY: %.2f\r\n", eyeTargetX, eyeTargetY);
  mySerCmd.Print(buf);

  sprintf(buf, "GAZE,%.2f,%.2f", eyeTargetX, eyeTargetY);
  Serial1.print(buf);
  Serial1.println();

  sendOK();
}

// -------------------------------------------------------
// setup()
// -------------------------------------------------------
void setup() {
  unsigned long serialTimout = millis();

  Serial1.begin(115200);
  while(!Serial1 && millis() - serialTimout <= 5000);

  serialTimout = millis();
  Serial.begin(115200);
  while(!Serial && millis() - serialTimout <= 5000);

  // Define serial commands
  mySerCmd.AddCmd("PING", SERIALCMD_FROMALL, send_PING);
  mySerCmd.AddCmd("VERSION", SERIALCMD_FROMALL, Get_Version);
  mySerCmd.AddCmd("H_GAZE", SERIALCMD_FROMALL, set_GAZE);

  // Initialize I2C (Slave Mode: address=0x5A)
  Wire.begin(AME_I2C_ADDRESS);
  Wire.onReceive(I2C_RxHandler);
  Wire.onRequest(I2C_TxHandler);

  // Initialize spectrum analyzer
  analogReadResolution(12);

  pinMode(STROBE, OUTPUT);
  pinMode(RESET, OUTPUT);
  pinMode(DC_One, INPUT);

  digitalWrite(STROBE, HIGH);
  digitalWrite(RESET, HIGH);
  
  digitalWrite(STROBE, LOW);
  delay(1);
  digitalWrite(RESET, HIGH);
  delay(1);
  digitalWrite(STROBE, HIGH);
  delay(1);
  digitalWrite(STROBE, LOW);
  delay(1);
  digitalWrite(RESET, LOW);

  // Initialize the 1x6 mouth Neopixel strip and clear the pixels
  pixels.begin();
  pixels.clear();

  // Initialize the onboard Neopixel
  onboard_pixel.begin();

  Serial.println("INFO: Starting application...");
}


// -------------------------------------------------------
// loop()
// -------------------------------------------------------
void loop() {
  unsigned long currentMillis = millis();

  Read_Frequencies();
  Graph_Frequencies();
  delay(50);

  if(currentMillis - previousMillis > blinkInterval) {
    previousMillis = currentMillis;

    if (ledState == LOW) {
      onboard_pixel.setPixelColor(0, onboard_pixel.Color(0, 10, 0));
      onboard_pixel.show();
      ledState = HIGH;
    } else {
      onboard_pixel.clear();
      onboard_pixel.show();
      ledState = LOW;
    }
  }

  ret = mySerCmd.ReadSer();
  if (ret == 0) {
    mySerCmd.Print((char *) "ERROR: Urecognized command\r\n");
  }
}


/*******************Pull frquencies from Spectrum Shield********************/
void Read_Frequencies(){
  //Read frequencies for each band
  for (freq_amp = 0; freq_amp < 7; freq_amp++)
  {
    digitalWrite(STROBE, HIGH);
    delayMicroseconds(50);
    digitalWrite(STROBE, LOW);
    delayMicroseconds(50);

    Frequencies_One[freq_amp] = analogRead(DC_One);
    Frequencies_One[freq_amp] = Frequencies_One[freq_amp] - 450;

    if (Frequencies_One[freq_amp] < 0) {
      Frequencies_One[freq_amp] = 0;
    }
  }
}

/*******************Light LEDs based on frequencies*****************************/
void Graph_Frequencies(){
  for(i = 0; i <= 5; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, map(Frequencies_One[i + 1], 0, 4095-450, 0, 255)));
  }
  pixels.show();
}

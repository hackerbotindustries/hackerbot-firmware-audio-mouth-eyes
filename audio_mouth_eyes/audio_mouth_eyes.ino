/****************************************************************************** 
Hackerbot Industries, LLC
Ian Bernstein
Created: April 2024
Updated: 2025.01.07

This sketch is written for the "Audio/Mouth/Eye" PCBA and controls the mouth of
hackerbot. It also acts as a command pass through to the eyes.

TODO - Add I2C Slave code so other parts of hackerbot can send commands change
modes for the mouth. Add a mode for raw control of the mouth.
*********************************************************************************/

#include <Adafruit_NeoPixel.h>
#include <SerialCmd.h>
#include <Wire.h>

// Audio Mouth Eyes software version
#define VERSION_NUMBER 2

// I2C address (0x5A)
#define I2C_ADDRESS 90

// I2C command addresses
// FIXME: need this to be sharable between projects - decide between a common library, a shared include directory (perhaps every sub-fw #include's a file from fw_main_controller?), or some other scheme
#define I2C_COMMAND_PING 0x01
#define I2C_COMMAND_VERSION 0x02
#define I2C_COMMAND_IDLE 0x08
#define I2C_COMMAND_LOOK 0x09

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
SerialCmd mySerCmd(Serial);


// I2C Rx Handler
void I2C_RxHandler(int numBytes) {
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


// -------------------------------------------------------
// setup()
// -------------------------------------------------------
void setup() {
  unsigned long serialTimout = millis();

  Serial.begin(115200);
  while(!Serial && millis() - serialTimout <= 5000);

  // Define serial commands
  mySerCmd.AddCmd("PING", SERIALCMD_FROMALL, send_PING);

  // Initialize I2C (Slave Mode: address=0x5A)
  Wire.begin(I2C_ADDRESS);
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
      onboard_pixel.setPixelColor(0, onboard_pixel.Color(0, 0, 10));
      onboard_pixel.show();
      ledState = HIGH;
    } else {
      onboard_pixel.clear();
      onboard_pixel.show();
      ledState = LOW;
    }
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

    //Serial.print(freq_amp);
    //Serial.print(": ");
    //Serial.print(Frequencies_One[freq_amp]);
    //Serial.print(" - ");
    //Serial.println(map(Frequencies_One[freq_amp], 0, 4095-450, 0, 255));
  }
}

/*******************Light LEDs based on frequencies*****************************/
void Graph_Frequencies(){
  for(i = 0; i <= 5; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, map(Frequencies_One[i + 1], 0, 4095-450, 0, 255)));
  }
  pixels.show();
}
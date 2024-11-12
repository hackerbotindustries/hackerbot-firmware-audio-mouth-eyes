/****************************************************************************** 
Hackerbot Industries, LLC
Ian Bernstein
Created: April 2024
Updated: 2024.11.11

This sketch is written for the "Audio/Mouth" PCB and controls the mouth of
hackerbot.

TODO - Add I2C Slave code so other parts of hackerbot can send commands change
modes for the mouth. Add a mode for raw control of the mouth.
*********************************************************************************/

#include <Adafruit_NeoPixel.h>

#define STROBE 2
#define RESET 3
#define DC_One A0
#define PIN 1
#define NUMPIXELS 6 // Popular NeoPixel ring size

#define DEBUG_SERIAL Serial

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel onboard_pixel(1, PIN_NEOPIXEL);

int freq_amp;
int Frequencies_One[7];
int i;

long previousMillis = 0;
int ledState = LOW;
long blinkInterval = 1000;

void setup() {
  //DEBUG_SERIAL.begin(115200);
  delay(50);
  //DEBUG_SERIAL.println("Mouth starting up...");
  analogReadResolution(12);


  pinMode(STROBE, OUTPUT);
  pinMode(RESET, OUTPUT);
  pinMode(DC_One, INPUT);

  digitalWrite(STROBE, HIGH);
  digitalWrite(RESET, HIGH);
  
  //Initialize Spectrum Analyzer
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
}


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
  for (freq_amp = 0; freq_amp<7; freq_amp++)
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

    //DEBUG_SERIAL.print(freq_amp);
    //DEBUG_SERIAL.print(": ");
    //DEBUG_SERIAL.print(Frequencies_One[freq_amp]);
    //DEBUG_SERIAL.print(" - ");
    //DEBUG_SERIAL.println(map(Frequencies_One[freq_amp], 0, 4095-450, 0, 255));
  }
}

/*******************Light LEDs based on frequencies*****************************/
void Graph_Frequencies(){
   for( i= 0; i<=5; i++)
   {
     pixels.setPixelColor(i, pixels.Color(0, 0, map(Frequencies_One[i], 0, 4095-450, 0, 255)));
   }
   pixels.show();
}
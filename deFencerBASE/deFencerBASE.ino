/*
  Arduino based Fencing Scoring detection
  https://github.com/Vilda007/ArduinoFencingScoringDetection

  Wiring:
  Fencer 1 hit detection (contact) - PIN D2 (+pulldown 100 kOhm)
  Fencer 2 hit detection (contact) - PIN D4 (+pulldown 100 kOhm)
  Fencer 1 hit signal LED (yellow) - PIN D5 (+pullup 330 Ohm)
  Fencer 2 hit signal LED (blue)   - PIN D6 (+pullup 330 Ohm)
  Ready LED (green)                - PIN D7 (+pullup 330 Ohm)
  Buzzer                           - PIN D8
  8x8 LED matrix display with MAX7219
    CLK                            - PIN D13 
    CS/LOAD                        - PIN D3
    DIN (Data IN)                  - PIN D11
    GND                            - GND
    VCC                            - +5 V

  This code has been heavily inspired by the examples you can find at
  https://www.arduino.cc/en/Tutorial/HomePage
*/

// Including the required Arduino libraries
#include "config.h"
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>



// Create a new instance of the MD_Parola class with hardware SPI connection
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

void setup() {
  // open the serial port at 9600 bps:
  Serial.begin(9600); 
  
  // initialize the LED pins as an output:
  pinMode(Fencer1LED, OUTPUT);
  digitalWrite(Fencer1LED, HIGH);
  pinMode(Fencer2LED, OUTPUT);
  digitalWrite(Fencer2LED, HIGH);
  pinMode(ReadyLED, OUTPUT);
  digitalWrite(ReadyLED, LOW);
  
  // initialize the pushbutton pin as an input:
  pinMode(Fencer1, INPUT);
  pinMode(Fencer2, INPUT);

  // Intialize the object for 8x8 LED display
  myDisplay.begin();

  // Set the intensity (brightness) of the 8x8 LED display (0 min - 15 max)
  myDisplay.setIntensity(3);

  // Clear the 8x8 LED display
  myDisplay.displayClear();
  myDisplay.displayScroll("0:0", PA_CENTER, PA_SCROLL_LEFT, ScrollSpeed);
}

//function Hit is called after the hit is detected
//void Hit(int FencerLED, char FencerHitSign[], int FencerHitSoundHz, int F1hits, int F2hits) { 
void Hit(int FencerLED, int FencerHitSoundHz, int F1hits, int F2hits) { 
  digitalWrite(ReadyLED, HIGH);
  digitalWrite(FencerLED, LOW);
  tone(BuzzerPin, FencerHitSoundHz, HitSoundDuration);
  int HitDisplayed = HitDisplayedDuration / 4;
  myDisplay.displayReset();
  myDisplay.displayClear();
  myDisplay.setTextAlignment(PA_CENTER);
  myDisplay.print(FencerHitSign);
  delay(HitDisplayed);
  myDisplay.setInvert(true);
  myDisplay.print(FencerHitSign);
  delay(HitDisplayed);
  myDisplay.setInvert(false);
  myDisplay.print(FencerHitSign);
  delay(HitDisplayed);
  myDisplay.setInvert(true);
  myDisplay.print(FencerHitSign);
  delay(HitDisplayed);
  myDisplay.setInvert(false);
  tone(BuzzerPin, ReadySoundHz, ReadySoundDuration);
  myDisplay.displayClear();
  (String(F1hits) + ":" + String(F2hits)).toCharArray(Score,10);
  Serial.println("--");
  Serial.println(Score);
  myDisplay.displayScroll(Score, PA_CENTER, PA_SCROLL_LEFT, ScrollSpeed);
  digitalWrite(FencerLED, HIGH);
  digitalWrite(ReadyLED, LOW);
}

void loop() {
  // read the state of the pushbutton value:
  Fencer1hit = digitalRead(Fencer1);
  Fencer2hit = digitalRead(Fencer2);
  //Serial.println(millis()); //just a possible check how long the loop takes

  // Fencer 1 scores 
  if (Fencer1hit == HIGH) { 
    Fencer1hits = Fencer1hits + 1;
    FencerHitSign = Fencer1HitSign;
    Hit(Fencer1LED, Fencer1HitSoundHz, Fencer1hits, Fencer2hits);
  }

  // Fencer 2 scores 
  if (Fencer2hit == HIGH) { 
    Fencer2hits = Fencer2hits + 1;
    FencerHitSign = Fencer2HitSign;
    Hit(Fencer2LED, Fencer2HitSoundHz, Fencer1hits, Fencer2hits);
  }

  //scroll the actual score over the 8x8 LED display
  if (myDisplay.displayAnimate()) {
    myDisplay.displayReset();
  }
  
}

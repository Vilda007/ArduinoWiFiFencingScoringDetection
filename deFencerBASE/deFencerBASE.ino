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
#include <painlessMesh.h>

// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;

bool calc_delay = false;
SimpleList<uint32_t> nodes;

void sendMessage() ; // Prototype
Task taskSendMessage( TASK_SECOND * 1, TASK_FOREVER, &sendMessage ); // start with a one second interval

// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;

// Create a new instance of the MD_Parola class with hardware SPI connection
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

void setup() {
  // open the serial port at 115200 bps:
  Serial.begin(115200);

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

  // initialize mesh
  mesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();

  blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []() {
    // If on, switch off, else switch on
    if (onFlag)
      onFlag = false;
    else
      onFlag = true;
    blinkNoNodes.delay(BLINK_DURATION);

    if (blinkNoNodes.isLastIteration()) {
      // Finished blinking. Reset task for next run
      // blink number of nodes (including this node) times
      blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
      // Calculate delay based on current mesh time and BLINK_PERIOD
      // This results in blinks between nodes being synced
      blinkNoNodes.enableDelayed(BLINK_PERIOD -
                                 (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
    }
  });
  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();

  randomSeed(analogRead(A0));

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
  (String(F1hits) + ":" + String(F2hits)).toCharArray(Score, 10);
  Serial.println("--");
  Serial.println(Score);
  myDisplay.displayScroll(Score, PA_CENTER, PA_SCROLL_LEFT, ScrollSpeed);
  digitalWrite(FencerLED, HIGH);
  digitalWrite(ReadyLED, LOW);
}

void loop() {
  mesh.update();
  digitalWrite(LED, !onFlag);

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

void sendMessage() {
  String msg = "Hello from node ";
  //msg += mesh.getNodeId();
  msg += MESH_NODE;
  msg += " myFreeMemory: " + String(ESP.getFreeHeap());
  mesh.sendBroadcast(msg);

  if (calc_delay) {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end()) {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }

  Serial.printf("Sending message: %s\n", msg.c_str());

  taskSendMessage.setInterval( random(TASK_SECOND * 1, TASK_SECOND * 5));  // between 1 and 5 seconds
}


void receivedCallback(uint32_t from, String & msg) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
  Serial.printf("--> startHere: New Connection, %s\n", mesh.subConnectionJson(true).c_str());
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  nodes = mesh.getNodeList();

  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
  calc_delay = true;
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay) {
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}

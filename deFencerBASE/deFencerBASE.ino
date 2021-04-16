/*
  Arduino based Fencing Scoring detection
  https://github.com/Vilda007/ArduinoWiFiFencingScoringDetection

  This code has been heavily inspired by the examples you can find at
  https://www.arduino.cc/en/Tutorial/HomePage
*/

// Including the required Arduino libraries
#include "config.h"
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
//#include <painlessMesh.h>
#include "namedMesh.h"

// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String & msg);
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

#define _TASK_PRIORITY
#define _TASK_TIMECRITICAL

Scheduler     lowts, ts; 
//painlessMesh  mesh;
namedMesh  mesh;

bool calc_delay = false;
SimpleList<uint32_t> nodes;

// Prototypes
void sendMessage() ; 
void BlockCallback();
void deBlockCallback();
void InvertDisplayYesCallback();
void InvertDisplayNoCallback();
void WeHaveWinnerCallback();
void ShowWinnerCallback();
void ShowScoreCallback();
void ResetScoreCallback();
//void AnimateOnDisplayCallback();

//Task taskSendMessage( TASK_SECOND * 1, TASK_FOREVER, &sendMessage ); // start with a one second interval

// Tasks
Task blinkNoNodes;  // Task to blink the number of nodes
Task Block(TASK_IMMEDIATE, TASK_FOREVER, &BlockCallback, &ts, false);
Task deBlock(BlockDuration * TASK_MILLISECOND, TASK_FOREVER, &deBlockCallback, &ts, false);
Task InvertDisplayYes(HitDisplayedDuration2 * TASK_MILLISECOND, TASK_FOREVER, &InvertDisplayYesCallback, &lowts, false);
Task InvertDisplayNo(HitDisplayedDuration2 * TASK_MILLISECOND, TASK_FOREVER, &InvertDisplayNoCallback, &lowts, false);
Task WeHaveWinner(TASK_IMMEDIATE, TASK_FOREVER, &WeHaveWinnerCallback, &lowts, false);
Task ShowWinner(TASK_IMMEDIATE, TASK_FOREVER, &ShowWinnerCallback, &lowts, false);
Task ShowScore(TASK_IMMEDIATE, TASK_FOREVER, &ShowScoreCallback, &lowts, false);
Task ResetScore(TASK_IMMEDIATE, TASK_FOREVER, &ResetScoreCallback, &lowts, false);
//Task AnimateOnDisplay(TASK_IMMEDIATE, TASK_FOREVER, &AnimateOnDisplayCallback, &lowts, false);
bool onFlag = false;

// Create a new instance of the MD_Parola class with hardware SPI connection
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

void setup() {
  // open the serial port at 115200 bps:
  Serial.begin(115200);

  lowts.setHighPriorityScheduler(&ts); 
  //lowts.enableAll(true); // this will recursively enable the higher priority tasks as well

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
  myDisplay.setIntensity(1);

  // Clear the 8x8 LED display
  myDisplay.displayClear();
  //myDisplay.displayScroll("0:0", PA_CENTER, PA_SCROLL_LEFT, ScrollSpeed);
  (String(Fencer1hits) + ":" + String(Fencer2hits)).toCharArray(Score, 10);
  Serial.println("--");
  Serial.println(Score);
  myDisplay.setTextAlignment(PA_CENTER);
  myDisplay.print(Score); 
  //myDisplay.displayScroll(Score, PA_CENTER, PA_SCROLL_LEFT, ScrollSpeed);
  //AnimateOnDisplay.enable();

  // initialize mesh
  mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION);  // set before init() so that you can see error messages
  mesh.init(MESH_SSID, MESH_PASSWORD, &lowts, MESH_PORT);
  mesh.setName(BaseNodeName); 
  //mesh.onReceive(&receivedCallback);
  mesh.onReceive([](uint32_t from, String &msg) {
    //Serial.printf("Received message by id from: %u, %s\n", from, msg.c_str());
  });

  mesh.onReceive([](String &from, String &msg) {
    Serial.printf("Received message by name from: %s, %s\n", from.c_str(), msg.c_str());
    if(msg == F1HitMsg){ //Fencer 1 scores
      Block.enable();
      FencerHitSign = Fencer1HitSign;
      InvertDisplayYes.enable();
      InvertDisplayNo.enableDelayed(HitDisplayedDuration4);
      tone(BuzzerPin, Fencer1HitSoundHz, HitSoundDuration);  
      Fencer1hits = Fencer1hits + 1;
      if (Fencer1hits >= CountTill){
        WinnerIs = 1;
        WeHaveWinner.enable();
      }
    }
    if(msg == F2HitMsg){ //Fencer 2 scores
      Block.enable();
      FencerHitSign = Fencer2HitSign;
      InvertDisplayYes.enable();
      InvertDisplayNo.enableDelayed(HitDisplayedDuration4);
      tone(BuzzerPin, Fencer2HitSoundHz, HitSoundDuration);  
      Fencer2hits = Fencer2hits + 1;
      if (Fencer2hits >= CountTill){
        WinnerIs = 2;
        WeHaveWinner.enable();
      }  
    }
  });
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);
  mesh.setRoot();
  mesh.setContainsRoot();

  //ts.addTask( taskSendMessage );
  //taskSendMessage.enable();

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
  lowts.addTask(blinkNoNodes);
  blinkNoNodes.enable();

  randomSeed(analogRead(A0));
}

void BlockCallback() {
    Block.disable();
    mesh.sendBroadcast(BlockMsg);
    Serial.println("----BLOCK-----");
    //deBlock.reset();
    deBlock.enableDelayed();
}

void deBlockCallback() {
    deBlock.disable();
    mesh.sendBroadcast(deBlockMsg);
    Serial.println("----deBLOCK-----");
    tone(BuzzerPin, ReadySoundHz, ReadySoundDuration);
    //myDisplay.displayClear();
    InvertDisplayYes.disable();
    InvertDisplayNo.disable();
    myDisplay.setInvert(false);
    (String(Fencer1hits) + ":" + String(Fencer2hits)).toCharArray(Score, 10);
    Serial.println("--");
    Serial.println(Score);
    //myDisplay.displayReset();
    //myDisplay.displayClear();
    myDisplay.print(Score);
    //myDisplay.displayScroll(Score, PA_CENTER, PA_SCROLL_LEFT, ScrollSpeed);
    //AnimateOnDisplay.enable();
}

void InvertDisplayYesCallback() {
  myDisplay.setInvert(true);
  myDisplay.print(FencerHitSign);
}

void InvertDisplayNoCallback() {
  myDisplay.setInvert(false);
  myDisplay.print(FencerHitSign);
}

void WeHaveWinnerCallback() {
  WeHaveWinner.disable();
  deBlock.disable();
  InvertDisplayYes.disable();
  InvertDisplayNo.disable();
  tone(BuzzerPin, ReadySoundHz, HitSoundDuration);
  (String(WinnerIs) + "won").toCharArray(Winner, 10);
  myDisplay.setInvert(true);
  myDisplay.print(Winner);
  ShowWinner.enableDelayed(5000);
}

void ShowWinnerCallback() {
  ShowWinner.disable();
  myDisplay.setInvert(false);
  myDisplay.print(Winner);
  ShowScore.enableDelayed(2000);
}

void ShowScoreCallback() {
  ShowScore.disable();
  myDisplay.setInvert(false);
  (String(Fencer1hits) + ":" + String(Fencer2hits)).toCharArray(Score, 10);
  myDisplay.print(Score);
  ResetScore.enableDelayed(6000);
}

void ResetScoreCallback() {
  ResetScore.disable();
  WinnerIs = 0;
  Fencer1hits = 0;
  Fencer2hits = 0;
  deBlock.enable();
}

/*
void AnimateOnDisplayCallback() {
  //scroll the actual score over the 8x8 LED display
  if (myDisplay.displayAnimate()) {
    myDisplay.displayReset();
  }
}
*/

/*
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
*/

void loop() {
  mesh.update();
  digitalWrite(LED, !onFlag);
}

void sendMessage() {
  String msg = "Hello from node ";
  //msg += mesh.getNodeId();
  msg += nodeName;
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

  //taskSendMessage.setInterval( random(TASK_SECOND * 1, TASK_SECOND * 5));  // between 1 and 5 seconds
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

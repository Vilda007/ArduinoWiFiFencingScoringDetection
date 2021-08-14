/*
  Arduino based Fencing Scoring detection
  https://github.com/Vilda007/ArduinoWiFiFencingScoringDetection

  This code has been heavily inspired by the examples you can find at
  https://www.arduino.cc/en/Tutorial/HomePage
*/

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMesh.h>
#include <TypeConversionFunctions.h>
#include <assert.h>
#include "config.h"
//#include <painlessMesh.h>
#include "namedMesh.h"

// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);
void CheckHitCallback();
void SendHitCallback();

#define _TASK_PRIORITY
#define _TASK_TIMECRITICAL

Scheduler     lowts, ts; // to control your personal task
//painlessMesh  mesh;
namedMesh  mesh;

bool calc_delay = false;
SimpleList<uint32_t> nodes;

void sendMessage() ; // Prototype
//Task taskSendMessage( TASK_SECOND * 1, TASK_FOREVER, &sendMessage ); // start with a one second interval

// Task to blink the number of nodes
Task blinkNoNodes;
Task CheckHit(TASK_IMMEDIATE, TASK_FOREVER, &CheckHitCallback, &ts, false);
Task SendHit(TASK_IMMEDIATE, TASK_FOREVER, &SendHitCallback, &ts, false);
bool onFlag = false;

void setup() {
  // open the serial port at 115200 bps:
  Serial.begin(115200);

  lowts.setHighPriorityScheduler(&ts);
  lowts.enableAll(true); // this will recursively enable the higher priority tasks as well

  // initialize the LED pins as an output:
  pinMode(GreenLED, OUTPUT);
  digitalWrite(GreenLED, HIGH);
  pinMode(RedLED, OUTPUT);
  digitalWrite(RedLED, HIGH);

  // initialize the pushbutton pin as an input:
  pinMode(Hit, INPUT);

  // initialize mesh
  mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION);  // set before init() so that you can see error messages
  mesh.init(MESH_SSID, MESH_PASSWORD, &lowts, MESH_PORT);
  mesh.setName(MyNodeName);
  //mesh.onReceive(&receivedCallback);
  mesh.onReceive([](uint32_t from, String & msg) {
    //Serial.printf("Received message by id from: %u, %s\n", from, msg.c_str());
  });

  mesh.onReceive([](String & from, String & msg) {
    Serial.printf("Received message by name from: %s, %s\n", from.c_str(), msg.c_str());
    if (msg == BlockMsg) {
      //double hit delay should come somewhere here
      CheckHit.disable();
      digitalWrite(RedLED, LOW);
      blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
    }
    if (msg == deBlockMsg) {
      CheckHit.enable();
      digitalWrite(RedLED, HIGH);
    }
  });
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

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
      NodesConnected = mesh.getNodeList().size();
      blinkNoNodes.setIterations((NodesConnected + 1) * 2);
      // Calculate delay based on current mesh time and BLINK_PERIOD
      // This results in blinks between nodes being synced
      if (NodesConnected < 2) {
        blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
      } else {
        blinkNoNodes.disable();
      }
    }
  });
  lowts.addTask(blinkNoNodes);
  blinkNoNodes.disable();
  CheckHit.disable();
  digitalWrite(RedLED, LOW);

  randomSeed(analogRead(A0));
}

void loop() {
  mesh.update();
  digitalWrite(LED, !onFlag);
}

void CheckHitCallback() {
  if (digitalRead(Hit) == HIGH) {
    CheckHit.disable();
    SendHit.enable();
    Serial.println(digitalRead(Hit));
  }
}

void SendHitCallback() {
  SendHit.disable();
  mesh.sendSingle(BaseNodeName, HitMsg);
  digitalWrite(GreenLED, LOW);
}


void sendMessage() {
  String msg = "Hello from node ";
  //msg += mesh.getNodeId();
  msg += nodeName;
  //msg += " myFreeMemory: " + String(ESP.getFreeHeap());
  //msg += " myMillis: " + String(millis());
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
  //blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

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

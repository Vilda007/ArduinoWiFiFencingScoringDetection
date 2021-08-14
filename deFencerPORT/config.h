// Definings
// Mesh
String BaseNodeName =     "FencerBase";   // Name needs to be unique  
String MyNodeName =       "Fencer2";      // Name needs to be unique
String OponentNodeName =  "Fencer1";      // Name needs to be unique
//String MyNodeName =       "Fencer1";      // Name needs to be unique
//String OponentNodeName =  "Fencer2";      // Name needs to be unique

String nodeName = MyNodeName;
#define   MESH_SSID       "FencerMesh"    // mesh name
#define   MESH_PASSWORD   "dArtagnan"     // mesh password
#define   MESH_PORT       5555            // mesh port (5555)

String HitMsg = String("Hit by ") + nodeName;
String BlockMsg = "BLOCK";
String deBlockMsg = "deBLOCK";

// some gpio pin that is connected to an LED...
// on my rig, this is 5, change to the right number of your LED.
#define   LED             D4       // GPIO number of connected LED, ON ESP-12 IS GPIO2

#define   BLINK_PERIOD    500 // milliseconds until cycle repeat (after hit)
#define   BLINK_DURATION  100  // milliseconds LED is on for

// PORT HW Definings
#define GreenLED D4     // Green LED PIN
#define RedLED D3       // Red LED PIN
#define Hit D0          // Hit (score) PIN

// variables will change:
int FencerHit = 0;                     // Fencer scores
boolean Blocked = true;               // Hits blocked
int NodesConnected = 0;                  // Number of nodes connected
int MeshEstablished = 0;                 // Are there 3 nodes connected? 0 = no | 1 = yes

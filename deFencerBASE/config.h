// Definings
// Mesh
#define   MESH_NODE       "Base"          // name of the node
//#define   MESH_NODE        "Fenc1"         // name of the node
//#define   MESH_NODE        "Fenc2"         // name of the node
#define   MESH_SSID       "FencerMesh"    // mesh name
#define   MESH_PASSWORD   "dArtagnan"     // mesh password
#define   MESH_PORT       5555            // mesh port (5555)

// some gpio pin that is connected to an LED...
// on my rig, this is 5, change to the right number of your LED.
#define   LED             D4       // GPIO number of connected LED, ON ESP-12 IS GPIO2

#define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for

// 8x8 LED display driver type hardware type
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
//#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW

#define MAX_DEVICES 1 // Number of 8x8 LED displays
#define CS_PIN D6     // Cable Select PIN (D6)
/*
 * CS  - MISO/D6
 * DIN - MOSI/D7
 * CLK - SCL/D5
 */

// constants 
// Advised PINs (in brackets) are for Wemos D1 R2 board
const int Fencer1 = D0;                  // Fencer 1 detection PIN (D0 pull-down needed 100 kOhm)
const int Fencer2 = D8;                  // Fencer 2 detection PIN (D2 internal 10k pull-down)
const int Fencer1LED = D3;               // Fencer 1 hit signal LED PIN (D3 internal 10k pull-up)
const int Fencer2LED = D1;               // Fencer 2 hit signal LED PIN (D4 pull-up needed 330 Ohm) 
const int ReadyLED = D4;                 // Green ready LED PIN (D4 internal 10k pull-up)
const int BuzzerPin = D2;                // Buzzer PIN (D2)
const String Fencer1HitSign = ">>";      // Fencer 1 hit sign for display
const String Fencer2HitSign = "<<";      // Fencer 2 hit sign for display
const int Fencer1HitSoundHz = 440;       // Fencer 1 hit sound pitch in Hz
const int Fencer2HitSoundHz = 880;       // Fencer 2 hit sound pitch in Hz
const int HitSoundDuration = 500;        // Hit sound duration in ms
const int HitDisplayedDuration = 2000;   // Hit sound duration in ms
const int ReadySoundHz = 220;            // Ready sound pitch in Hz
const int ReadySoundDuration = 50;       // Ready sound duration in ms
const int ScrollSpeed =  200;            // Speed of scrolling in ms

// variables will change:
int Fencer1hit = 0;                      // Fencer 1 scores
int Fencer2hit = 0;                      // Fencer 2 scores
int Fencer1hits = 0;                     // Fencer 1 score (hit counter)
int Fencer2hits = 0;                     // Fencer 2 score (hit counter)
char Score[10]  = "0:0";                 // Score to display
String FencerHitSign = "";               // Hit sign to display

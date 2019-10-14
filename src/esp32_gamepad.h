//
// ESP32 Gamepad library
// written by Larry Bank
// Project started 10/11/2019
//
#ifndef __ESP32_GAMEPAD__
#define __ESP32_GAMEPAD__

typedef struct tag_ss_gamepad {
uint16_t u16Buttons; // button bits (1 = pressed)
int iLJoyX, iLJoyY; // left analog stick (-128 to 127)
int iRJoyX, iRJoyY; // right analog stick
} SS_GAMEPAD;

// Callback events
enum {
  EVENT_DISCONNECT=1,
  EVENT_CONNECT,
  EVENT_CONTROLCHANGE // a button or analog stick changed
};

// Callback function prototype
typedef void (SS_CALLBACK)(int iEvent, SS_GAMEPAD * pGamepad);

//
// Register your callback function to be notified of controller changes
// This is called from the bluetooth stack, so don't do system calls
// from within this function.
//
void SS_RegisterCallback(SS_CALLBACK *myCallback);

// Initialize the library
void SS_Init(void);

//
// Scan for a valid device
// Scan for iTime (1-30) seconds
// Returns immediately if a valid device is found
// Returns TRUE along with a 6-byte bluetooth address if a SteelSeries:Free is found
// FALSE if not found or invalid parameters
//
bool SS_Scan(int iTime, uint8_t *bdAddr);

// Connect to the given bluetooth address
// returns TRUE for success, FALSE for failure
bool SS_Connect(uint8_t *addr);

#endif // __ESP32_GAMEPAD__


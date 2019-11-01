//
// ESP32 gamepad demo
//
// Tested on the TTGO T-Display, M5Stick-C and Mike Rankin's color coincell board V2
// Copyright (c) 2019 BitBank Software, Inc.
// written by Larry Bank
// bitbank@pobox.com
//
// Makes use of my bb_spi_lcd library (found in the Arduino library manager)
#include <esp32_gamepad.h>
#include <bb_spi_lcd.h>
#ifdef ARDUINO_M5Stick_C
#include <M5StickC.h>
#endif

// Uncomment this line if compiling for the TTGO T-Display
//#define TTGO_T_DISPLAY

#ifdef TTGO_T_DISPLAY
int GetButtons(void)
{
static int iOldState;
int iState, iPressed;

  iState = 0;
  if (digitalRead(0) == LOW)
    iState |= 1;
  if (digitalRead(35) == LOW)
    iState |= 2;
       
  // Test button bits for which ones changed from LOW to HIGH
  iPressed = (iState ^ iOldState) & iState; // tells us which ones just changed to 1
  iOldState = iState;
  iPressed = iPressed | (iState << 8); // prepare combined state
  return iPressed; 
} /* GetButtons() */
#endif

#ifdef ARDUINO_M5Stick_C
int GetButtons(void)
{
static int iOldState;
int iState, iPressed;

  iState = 0;
  if (digitalRead(39) == LOW)
    iState |= 1;
  if (digitalRead(37) == LOW)
    iState |= 2;
       
  // Test button bits for which ones changed from LOW to HIGH
  iPressed = (iState ^ iOldState) & iState; // tells us which ones just changed to 1
  iOldState = iState;
  iPressed = iPressed | (iState << 8); // prepare combined state
  return iPressed; 
} /* GetButtons() */
#endif

// Mike Rankin's Color Coincell V2
#if !defined ( ARDUINO_M5Stick_C ) && !defined( TTGO_T_DISPLAY )
// Touch Pad1 = GPIO27, T7
// Touch Pad2 = GPIO12, T5 
// Touch Pad3 = GPIO15, T3
// Touch Pad4 = GPIO2 , T2
#define BUTTON_THRESHOLD 98
uint8_t iButtons[4] = {T7,T5,T3,T2};
int GetButtons(void)
{
static int iOldState;
int iState, iPressed;
int i, j;
int iCounts[4] = {0};

  iState = 0;
  // remove the 'flicker' of the buttons
  for (j=0; j<5; j++)
  {
    for (i=0; i<4; i++)
    {
      if (touchRead(iButtons[i]) < BUTTON_THRESHOLD)
        iCounts[i]++;
    }
    delay(4);
  }
  for (i=0; i<4; i++)
    if (iCounts[i] == 5)
       iState |= (1<<i);
       
    // Test button bits for which ones changed from LOW to HIGH
    iPressed = (iState ^ iOldState) & iState; // tells us which ones just changed to 1
    iOldState = iState;
    iPressed = iPressed | (iState << 8); // prepare combined state
    return iPressed; 
} /* GetButtons() */
#endif // !TTGO_T_DISPLAY && !M5StickC

SS_GAMEPAD gp;
uint8_t address[6];
volatile bool bChanged;

void SS_Callback(int iEvent, SS_GAMEPAD *pGamepad)
{
  bChanged = 1;
  memcpy(&gp, pGamepad, sizeof(SS_GAMEPAD));
} /* SS_Callback() */

void setup() {
int i;
uint8_t devAddr[6];
int bDone = 0;

#ifdef TTGO_T_DISPLAY
  spilcdInit(LCD_ST7789_135, 0, 0, 0, 32000000, 5, 16, -1, 4, -1, 19, 18); // TTGO T-Display pin numbering, 40Mhz
  pinMode(0, INPUT_PULLUP);
  pinMode(35, INPUT_PULLUP);
#endif
#ifdef ARDUINO_M5Stick_C
  M5.begin();
  M5.Axp.ScreenBreath(11); // turn on backlight
  spilcdInit(LCD_ST7735S, 0, 1, 0, 40000000, 5, 23, 18, -1, 19, 15, 13); // M5StickC pin numbering
#endif
// Mike Rankin's Color coincell board v2
#if !defined(ARDUINO_M5Stick_C) && !defined(TTGO_T_DISPLAY)
  spilcdInit(LCD_ST7735S_B, 1, 1, 0, 32000000, 4, 21, 22, 26, -1, 23, 18); // Mike Rankin's color coin cell pin numbering
#endif
   spilcdSetOrientation(LCD_ORIENTATION_ROTATED);

  SS_Init();
  SS_RegisterCallback(SS_Callback);
try_again:
  spilcdFill(0,1);
  spilcdWriteString(0,0,(char *)"Starting discovery...", 0xffff, 0, FONT_NORMAL, 1);
  if (SS_Scan(6, devAddr))
  {    
    spilcdWriteString(0,16,(char *)"SS found!", 0x6e0, 0, FONT_STRETCHED, 1);
    if (SS_Connect(devAddr))
    {
      spilcdWriteString(0,32,(char *)"Connected!", 0x6e0, 0, FONT_STRETCHED, 1);
      bDone = 1;
      delay(1000);
    }
    else
      spilcdWriteString(0,32,(char *)"Not Connected", 0xf800, 0, FONT_STRETCHED, 1);
  }
  else
  {
    spilcdWriteString(0,16,(char *)"Not found", 0xf800, 0, FONT_STRETCHED, 1);
  }
  if (!bDone)
  {
    spilcdWriteString(0,64,(char *)"press any key to continue", 0x7ff, 0, FONT_SMALL, 1);
    while (GetButtons() == 0)
    {
     
    }
    goto try_again;
  }
} // setup()
 
void loop() {
char szTemp[32];

  spilcdFill(0,1);
  while(1)
  {
    sprintf(szTemp,"B 0x%04x", gp.u16Buttons);
    spilcdWriteString(0,0,szTemp, 0xffff,0,FONT_STRETCHED, 1);
    sprintf(szTemp,"L%04d,%04d", gp.iLJoyX,gp.iLJoyY);
    spilcdWriteString(0,24,szTemp, 0xf81f,0,FONT_STRETCHED, 1); 
    sprintf(szTemp,"R%04d,%04d", gp.iRJoyX,gp.iRJoyY);
    spilcdWriteString(0,48,szTemp, 0x6e0,0,FONT_STRETCHED, 1); 
  }
} /* loop() */

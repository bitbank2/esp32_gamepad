//
// esp32_gamepad - connect SteelSeries:Free to esp32
// Written by Larry Bank
// Copyright (c) 2019 BitBank Software, Inc.
// Project started 10/11/2019
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
#include "esp_gap_bt_api.h"
#include <BluetoothSerial.h>
#include "esp32_gamepad.h"

static uint8_t bFound, address[6];

SS_CALLBACK *pCallback = NULL; // user callback function for controller changes

static esp_err_t connHan;
static SS_GAMEPAD gamepad; // local copy of controls
static uint8_t bZeemote; // Zeemote or SteelSeries:Free
static uint32_t handle; // connection handle;

typedef struct _bd_data {
             esp_spp_status_t    status;         /*!< status */
             uint32_t            handle;         /*!< The connection handle */
             uint16_t            len;            /*!< The length of data */
             uint8_t             *data;          /*!< The data received */
         } bd_data;

BluetoothSerial SerialBT;

//
// Register your callback function to be notified of controller changes
// This is called from the bluetooth stack, so don't do system calls
// from within this function.
//
void SS_RegisterCallback(SS_CALLBACK *myCallback)
{
  pCallback = myCallback;
} /* SS_RegisterCallback() */

//
// Disconnect the gamepad
//
void SS_Disconnect(void)
{
  if (handle != -1)
    esp_spp_disconnect(handle);
} /* SS_Disconnect() */

// Get the latest button and analog stick state
// from the HID-style packet received
static void ProcessZeemotePacket(int len, uint8_t *pData)
{
int i;

  switch (pData[2])
  {
    case 7: // buttons
      gamepad.u16Buttons &= 0xfff0; // get rid of lower 4 button states
      for (i=0; i<4; i++)
      {
        if (pData[3+i] != 0xfe) // button is pressed
           gamepad.u16Buttons |= (1<<pData[3+i]);
      }
      break;
    case 0x1c: // d-pad and new buttons
      gamepad.u16Buttons &= 0xf; // remove upper 8 bits
      for (i=0; i<8; i++)
      {
        if (pData[3+i] >= 4 && pData[3+i] <= 11) // valid values
           gamepad.u16Buttons |= (1<<pData[3+i]);
      }
      break;
    case 8: // analog sticks
      if (pData[3] == 0) // left stick
      {
        gamepad.iLJoyX = (int8_t)pData[4];
        gamepad.iLJoyY = (int8_t)pData[5];
      }
      else // must be right
      {
        gamepad.iRJoyX = (int8_t)pData[4];
        gamepad.iRJoyY = (int8_t)pData[5];
      }
      break;
    case 11: // battery (don't really care for now)
      break;
  }
} /* ProcessZeemotePacket() */

static void spp_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  if (event == ESP_SPP_DATA_IND_EVT) // data received
  {
    // the only one we care about
    bd_data *bd = (bd_data *)param;
    handle = bd->handle;
    ProcessZeemotePacket(bd->len, bd->data);
    if (pCallback)
      (*pCallback)(EVENT_CONTROLCHANGE, &gamepad);
  }
  else if (event == ESP_SPP_CLOSE_EVT) // disconnected
  {
    handle = -1;
    if (pCallback)
      (*pCallback)(EVENT_DISCONNECT, &gamepad);
  }
  else if (event == ESP_SPP_OPEN_EVT) // connected
  {
    if (pCallback)
      (*pCallback)(EVENT_CONNECT, &gamepad);
  }
//  Serial.printf("Callback event = %d\n", (int)event);  
} /* spp_callback() */

// We only need this callback for device discovery
static void gap_callback (esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* param)
{
uint8_t Addr[6];
int i, iCount;
//disc_res_param *disc_res = (disc_res_param *)param;

  if (event == ESP_BT_GAP_DISC_RES_EVT) // discovery result
  {
     memcpy(Addr, param->disc_res.bda, 6);
//     Serial.printf("Discovery result: %02x:%02x:%02x:%02x:%02x:%02x\n",
//     Addr[0],Addr[1],Addr[2],Addr[3],Addr[4],Addr[5]);
     if (Addr[0] == 0x00 && Addr[1] == 0x1c && Addr[2] == 0x4d) // Zeemote
     {
       bFound = 1;
       bZeemote = 1;
       memcpy(address, Addr, 6);
     }
     if (Addr[0] == 0x28 && Addr[1] == 0x9a && Addr[2] == 0x4b) // SteelSeries:Free
     {
       bFound = 1;
       bZeemote = 0;
       memcpy(address, Addr, 6);
     }
  }
//  else
//     Serial.printf("gap callback event = %d\n", (int)event);
} /* gap_callback() */

// Initialize the library
void SS_Init(void)
{
  handle = -1;
  SerialBT.begin("ESP32", true); // Apparently this needs to be called first to initialize bluetooth (doesn't matter master or slave)
  esp_bt_gap_register_callback(gap_callback);
  esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
} /* SS_Init() */

// Scan for a valid device
bool SS_Scan(int iTime, uint8_t *bdAddr)
{
int i;

  if (iTime < 1 || iTime > 30 || bdAddr == NULL)
     return false; // invalid parameters
     
  esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
  bFound = i = 0;
  // search for up to N seconds or until we find a valid controller
  while (i < iTime*4 && !bFound)
  {
    delay(250);
    i++;
  }
  esp_bt_gap_cancel_discovery();
  if (bFound)
  {
    memcpy(bdAddr, address, 6); // return the bluetooth address we found
    return true;
  }
  return false;
} /* SS_Scan() */

// Connect to the given bluetooth address
bool SS_Connect(uint8_t *addr)
{
    esp_spp_init(ESP_SPP_MODE_CB);
    esp_spp_register_callback(spp_callback);
    connHan = esp_spp_connect(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 1, addr); // only channel 1 works
    return (connHan == ESP_OK);
  
} /* SS_Connect() */


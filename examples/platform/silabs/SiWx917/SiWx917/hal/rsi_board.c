/*******************************************************************************
* @file  rsi_board.c
* @brief 
*******************************************************************************
* # License
* <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
*******************************************************************************
*
* The licensor of this software is Silicon Laboratories Inc. Your use of this
* software is governed by the terms of Silicon Labs Master Software License
* Agreement (MSLA) available at
* www.silabs.com/about-us/legal/master-software-license-agreement. This
* software is distributed to you in Source Code format and is governed by the
* sections of the MSLA applicable to Source Code.
*
******************************************************************************/

#include "rsi_chip.h"
#include "rsi_board.h"
#include "rsi_ccp_common.h"
#include <stdio.h>

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
typedef struct {
  uint8_t port;
  uint8_t pin;
} PORT_PIN_T;

#ifdef BRD4325A
static const PORT_PIN_T ledBits[] = { { 0, 10 }, { 0, 8 } };
#else
#ifdef REV_1P2_CHIP
static const PORT_PIN_T ledBits[] = { { 0, 0 }, { 0, 1 }, { 0, 2 } };
#else
static const PORT_PIN_T ledBits[] = { { 0, 0 }, { 0, 2 }, { 0, 12 } };
#endif
#endif
static const uint32_t ledBitsCnt = sizeof(ledBits) / sizeof(PORT_PIN_T);

/**
 * @fn        void RSI_Board_Init(void)
 * @brief     Set up and initialize all required blocks and functions related to the board hardware.
 * @return    none
 */
void RSI_Board_Init(void)
{
  uint32_t i;
#ifdef BRD4325A
  for (i = 0; i < ledBitsCnt; i++) {
    if (i == 0) {
      RSI_EGPIO_PadSelectionEnable(5);
      /*Set the GPIO pin MUX */
      RSI_EGPIO_SetPinMux(EGPIO, ledBits[i].port, ledBits[i].pin, 0);
      /*Set GPIO direction*/
      RSI_EGPIO_SetDir(EGPIO, ledBits[i].port, ledBits[i].pin, 0);
    } else {
      /*Set the GPIO pin MUX */
      RSI_EGPIO_SetPinMux(EGPIO1, ledBits[i].port, ledBits[i].pin, 0);
      /*Set GPIO direction*/
      RSI_EGPIO_SetDir(EGPIO1, ledBits[i].port, ledBits[i].pin, 0);
    }
  }
#else
  for (i = 0; i < ledBitsCnt; i++) {
    /*Set the GPIO pin MUX */
    RSI_EGPIO_SetPinMux(EGPIO1, ledBits[i].port, ledBits[i].pin, 0);
    /*Set GPIO direction*/
    RSI_EGPIO_SetDir(EGPIO1, ledBits[i].port, ledBits[i].pin, 0);
  }
#endif
  /*Enable the DEBUG UART port*/
  return;
}

/**
 * @fn          void RSI_Board_LED_Set(int x, int y)
 * @brief       Sets the state of a board LED to on or off.
 * @param[in]   x    :  LED number to set state for
 * @param[in]   y     :  true for on, false for off
 * @return      none
 */
void RSI_Board_LED_Set(int x, int y)
{
#ifdef BRD4325A
  if (x == 0) {
    RSI_EGPIO_SetPin(EGPIO, (uint8_t)ledBits[x].port, (uint8_t)ledBits[x].pin, (uint8_t)y);
  } else if (x == 1) {
    RSI_EGPIO_SetPin(EGPIO1, (uint8_t)ledBits[x].port, (uint8_t)ledBits[x].pin, (uint8_t)y);
  }
#else
  RSI_EGPIO_SetPin(EGPIO1, ledBits[x].port, ledBits[x].pin, y);
#endif
  return;
}

/**
 * @fn          void RSI_Board_LED_Toggle(int x)
 * @brief       Toggles the current state of a board LED.
 * @param[in]   x : LED number to change state for
 * @return      none
 */
void RSI_Board_LED_Toggle(int x)
{
#ifdef BRD4325A
  if (x == 0) {
    RSI_EGPIO_TogglePort(EGPIO, ledBits[x].port, (1 << ledBits[x].pin));
  } else if (x == 1) {
    RSI_EGPIO_TogglePort(EGPIO1, ledBits[x].port, (1 << ledBits[x].pin));
  }
#else
  RSI_EGPIO_TogglePort(EGPIO1, ledBits[x].port, (1 << ledBits[x].pin));
#endif
  return;
}

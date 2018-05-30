/**
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2016 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * crazycar.c - Deck driver for the Crazyflie 2.0 Crazycar deck
 * ----------------------------------------------------------------------------
 * Add this to the build by adding the following to the Makefile:
 * PROJ_OBJ_CF2 += crazycar.o
 * CFLAGS += -DDECK_FORCE=bcCrazycar
 * This file goes in:
 * crazyflie-firmware/src/deck/drivers/src
 * 
 * Note: Had to disable Werror in Makefile to get it to compile
 */

#include <stdint.h>
#include <stdlib.h>
#include "stm32fxxx.h"

#include "deck.h"

#include "FreeRTOS.h"
#include "timers.h"
#include "uart2.h"
#include "debug.h"
#include "log.h"

static int logIdRoll;
static int logIdPitch;
// https://wiki.bitcraze.io/projects:crazyflie:crtp:commander
// probably:
static int logIdYaw
static int logIdThrust
// Still need to find digital inputs for buttons etc

/* Template data for controlling the Skeleton bot */
static char data[] = {'S', 'S', '0', '0', 0x0A};
// static char data[] = {'S', 'S', ,,}

/* Create output string for sending to the Skeleton bot */
void carSetControl(float roll, float pitch)
{
  float f = pitch / 30.0;
  float t = roll / 60.0;

  float m1 = f + t;
  float m2 = f - t;

  if (m1 > 1.0)
    m1 = 1.0;
  if (m1 < -1)
    m1 = -1;

  if (m2 > 1.0)
    m2 = 1.0;
  if (m2 < -1)
    m2 = -1;

  if (m1 > 0)
  {
    data[0] = 'F';
    data[2] = (int)(m1 * 10) + '0';
  } else {
    data[0] = 'B';
    data[2] = (int)(m1 * -10) + '0';
  }

  if (m2 > 0)
  {
    data[1] = 'F';
    data[3] = (int)(m2 * 10) + '0';
  } else {
    data[1] = 'B';
    data[3] = (int)(m2 * -10) + '0';
  }
}


/* Send commands to Skeleton bot at 10Hz */
static xTimerHandle timer;
static void ctrlTimer(xTimerHandle timer)
{
    carSetControl(logGetFloat(logIdRoll), logGetFloat(logIdPitch));
    uart2SendData(sizeof(data), (uint8_t*)data);
}

/* Initialize the deck driver */
static void crazycarDeckInit(DeckInfo *info)
{
    logIdRoll = logGetVarId("ctrltarget", "roll");
    logIdPitch = logGetVarId("ctrltarget", "pitch");

    uart2Init(19200);
    timer = xTimerCreate( "ctrlTimer", M2T(100),
                           pdTRUE, NULL, ctrlTimer );
    xTimerStart(timer, 100);
}

static const DeckDriver crazycar_deck = {
  .vid = 0xBC,
  .pid = 0x00,
  .name = "bcCrazycar",

  .usedPeriph = DECK_USING_UART2,
  .usedGpio = DECK_USING_TX2 | DECK_USING_RX2,

  .init = crazycarDeckInit
};

DECK_DRIVER(crazycar_deck);
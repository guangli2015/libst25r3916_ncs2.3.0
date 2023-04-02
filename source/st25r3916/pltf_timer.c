/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <zephyr/kernel.h>

#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "pltf_timer.h"



uint32_t platformGetSysTick_zephyr()
{
	return k_uptime_get_32();
}


/*******************************************************************************/
uint32_t timerCalculateTimer( uint16_t time )
{  
  return (platformGetSysTick() + time);
}


/*******************************************************************************/
bool timerIsExpired( uint32_t timer )
{
  uint32_t uDiff;
  int32_t sDiff;
  
  uDiff = (timer - platformGetSysTick());   /* Calculate the diff between the timers */
  sDiff = uDiff;                            /* Convert the diff to a signed var      */
  /* Having done this has two side effects: 
   * 1) all differences smaller than -(2^31) ms (~25d) will become positive
   *    Signaling not expired: acceptable!
   * 2) Time roll-over case will be handled correctly: super!
   */
  
  /* Check if the given timer has expired already */
  if( sDiff < 0 )
  {
    return true;
  }
  
  return false;
}


/*******************************************************************************/
void timerDelay( uint16_t tOut )
{
  uint32_t t;
  
  /* Calculate the timer and wait blocking until is running */
  t = timerCalculateTimer( tOut );
  while( timerIsRunning(t) );
}


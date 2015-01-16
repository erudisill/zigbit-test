/**
 * \file main.c
 *
 * \brief zigbit-test
 *
 * Copyright (C) 2015 CP Handheld Technologies
 *
  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/boot.h>

#include "config.h"
#include "hal.h"
#include "sys.h"
#include "sysTimer.h"
#include "halUart.h"
#include "halLed.h"


//#define TEST_SENDER
#define TEST_RECEIVER


#ifdef TEST_RECEIVER

SYS_Timer_t statusTimer;
uint32_t received_count;
uint32_t received_bytes;
uint32_t ticks_count;
uint32_t received_count_prev;
uint16_t received_delta;
char buffer[50];

void statusTimerHandler(SYS_Timer_t *timer)
{
	ticks_count += 1;
	received_delta = received_count - received_count_prev;
	received_count_prev = received_count;
	sprintf(buffer, "%lu\tR:%lu (+%d)\tB:%lu\r\n", ticks_count, received_count, received_delta, received_bytes);
	for (int i=0; buffer[i]; HAL_UartWriteByte(buffer[i++]));
	(void)timer;
}

bool appDataInd(NWK_DataInd_t *ind)
{
	HAL_LedOn(LED_DATA);
	received_count += 1;
	received_bytes += ind->size;
	HAL_LedOff(LED_DATA);
	return true;
}

void APP_setup(void)
{
	statusTimer.interval = 1000;
	statusTimer.mode = SYS_TIMER_PERIODIC_MODE;
	statusTimer.handler = statusTimerHandler;
	SYS_TimerStart(&statusTimer);

	NWK_OpenEndpoint(1, appDataInd);
}

#endif

void APP_setupNetwork(void)
{
	NWK_SetAddr(0x0000);
	NWK_SetPanId(0x1973);
	PHY_SetChannel(0x16);
	PHY_SetRxState(true);
}

void HAL_UartBytesReceived(uint16_t bytes)
{

}

void APP_TaskHandler(void)
{

}

int main(void)
{
  SYS_Init();
  HAL_LedInit();
  HAL_UartInit(38400);
  APP_setupNetwork();
  APP_setup();

  while (1)
  {
    SYS_TaskHandler();
    HAL_UartTaskHandler();
    APP_TaskHandler();
  }
}

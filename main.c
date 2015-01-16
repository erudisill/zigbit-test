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

SYS_Timer_t statusTimer;


#define TEST_SENDER
//#define TEST_RECEIVER


#ifdef TEST_SENDER

char msg[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789CPHT";
NWK_DataReq_t nwkDataReq;

void appDataConf(NWK_DataReq_t *req)
{
	HAL_LedOff(LED_DATA);
	SYS_TimerStart(&statusTimer);
}

void send(void)
{
	if (NWK_Busy()) {
		SYS_TimerStart(&statusTimer);
		return;
	}

	HAL_LedOn(LED_DATA);

	nwkDataReq.dstAddr = 0;
	nwkDataReq.dstEndpoint = 1;
	nwkDataReq.srcEndpoint = 1;
	nwkDataReq.options = 0;
	nwkDataReq.data = (uint8_t *)msg;
	nwkDataReq.size = sizeof(msg);
	nwkDataReq.confirm = appDataConf;

	NWK_DataReq(&nwkDataReq);
}

void statusTimerHandler(SYS_Timer_t *timer)
{
	send();
	(void)timer;
}

bool appDataInd(NWK_DataInd_t *ind)
{
	return true;
}

void APP_setup(void)
{
	statusTimer.interval = 50;
	statusTimer.mode = SYS_TIMER_INTERVAL_MODE;
	statusTimer.handler = statusTimerHandler;
	SYS_TimerStart(&statusTimer);

	NWK_OpenEndpoint(1, appDataInd);
}

#endif



#ifdef TEST_RECEIVER

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
#ifdef TEST_SENDER
	uint8_t b;
	uint16_t short_addr;
	b = boot_signature_byte_get(0x0102);	short_addr	 = b;
	b = boot_signature_byte_get(0x0103);	short_addr	|= ((uint16_t)b << 8);
	NWK_SetAddr(short_addr);
#else
	NWK_SetAddr(0x0000);
#endif

	NWK_SetPanId(0x1973);
	PHY_SetChannel(0x16);
	PHY_SetTxPower(0);
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

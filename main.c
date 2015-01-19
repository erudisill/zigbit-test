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

#include "messages.h"

SYS_Timer_t statusTimer;
char msg[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789CPHT";

//#define TEST_SENDER

#define TEST_RECEIVER
#define RECEIVER_USE_QUEUE
#define RECEIVER_DEQUEUE_PER_TICK 80
#define RECEIVER_MAX_IN_QUEUE_BEFORE_DUMP 80
#define RECEIVER_DUMP_TO_USART
//#define RECEIVER_TICKS_RECEIVING
#define RECEIVER_TICKS_DUMPING

#define SEND_INTERVAL 10

#ifdef TEST_SENDER

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
	statusTimer.interval = SEND_INTERVAL;
	statusTimer.mode = SYS_TIMER_INTERVAL_MODE;
	statusTimer.handler = statusTimerHandler;
	SYS_TimerStart(&statusTimer);

	NWK_OpenEndpoint(1, appDataInd);
}

void APP_TaskHandler(void)
{

}
#endif

#ifdef TEST_RECEIVER

uint32_t received_count;
uint32_t received_bytes;
uint32_t secs_count;
uint32_t ticks_count;
uint32_t ticks_count_prev;
uint16_t ticks_count_delta;
uint32_t received_count_prev;
uint16_t received_delta;
uint32_t error_count;
uint32_t error_count_prev;
uint16_t error_delta;
uint32_t error_queue_count;
uint32_t error_queue_count_prev;
uint16_t error_queue_delta;
uint16_t dequeue_count;
uint8_t checksum;
bool signal_status;
char buffer[128];
message_t message;
enum {
	STATE_RECEIVING, STATE_DUMPING
};
uint8_t current_state;

void printStatus(void) {
	int queue_count;
	int recs_per_sec;

	received_delta = received_count - received_count_prev;
	received_count_prev = received_count;

	error_delta = error_count - error_count_prev;
	error_count_prev = error_count;

	error_queue_delta = error_queue_count - error_queue_count_prev;
	error_queue_count_prev = error_queue_count;

	ticks_count_delta = ticks_count - ticks_count_prev;
	ticks_count_prev = ticks_count;

	queue_count = messages_count();
	recs_per_sec = received_count / secs_count;

	sprintf(buffer,
			"\r\n%lu\tR:%lu (+%d)\tRps:%d\tE:%lu (+%d)\tQ:%d\tQe:%lu (+%d)\tD:%d\tB:%lu\tT:%d\t",
			secs_count, received_count, received_delta, recs_per_sec, error_count,
			error_delta, queue_count, error_queue_count, error_queue_delta,
			dequeue_count, received_bytes, ticks_count_delta);
	for (int i = 0; buffer[i]; HAL_UartWriteByte(buffer[i++]))
		;

	dequeue_count = 0;
}

void statusTimerHandler(SYS_Timer_t *timer) {

	//printStatus();
	secs_count += 1;
	signal_status = true;
	(void) timer;
}

bool appDataInd(NWK_DataInd_t *ind) {
	bool result;

	HAL_LedOn(LED_DATA);

	received_count += 1;
	received_bytes += ind->size;

	// Validate data
	if (ind->size != sizeof(msg)) {
		error_count += 1;
	} else {
		uint8_t cs = 0;
		for (int i = 0; i < ind->size; cs ^= ind->data[i++])
			;
		if (cs != checksum) {
			error_count += 1;
		}
#ifdef RECEIVER_USE_QUEUE
		else {
			memset(message.data, 0, sizeof(message.data));
			memcpy(message.data, ind->data, ind->size);
			result = messages_enqueue(&message);
			if (result == false) {
				error_queue_count += 1;
			}
		}
#endif
	}

	HAL_LedOff(LED_DATA);
	return true;
}

void APP_setup(void) {
	checksum = 0;
	for (int i = 0; i < sizeof(msg); checksum ^= msg[i++])
		;

	statusTimer.interval = 1000;
	statusTimer.mode = SYS_TIMER_PERIODIC_MODE;
	statusTimer.handler = statusTimerHandler;
	SYS_TimerStart(&statusTimer);

	NWK_OpenEndpoint(1, appDataInd);

	current_state = STATE_RECEIVING;
}

void APP_TaskHandler(void) {
	bool result;
	message_t dump;
	int c;

	switch (current_state) {
	case STATE_RECEIVING:
#ifdef RECEIVER_TICKS_RECEIVING
		ticks_count += 1;
#endif
		c = messages_count();
		if (c >= RECEIVER_MAX_IN_QUEUE_BEFORE_DUMP) {
			PHY_SetRxState(false);
			current_state = STATE_DUMPING;
		}
		break;
	case STATE_DUMPING:
		if (!NWK_Busy()) {
#ifdef RECEIVER_TICKS_DUMPING
			ticks_count += 1;
#endif
			if (c > RECEIVER_DEQUEUE_PER_TICK)
				c = RECEIVER_DEQUEUE_PER_TICK;

			for (int i = 0; i < c; i++) {
				memset(&dump, 0, sizeof(message_t));
				result = messages_dequeue(&dump);
				if (result == false)
					break;
				dequeue_count += 1;

#ifdef RECEIVER_DUMP_TO_USART
				// Print the data
				HAL_UartWriteByte('\r');
				HAL_UartWriteByte('\n');
				for (int i=0; dump.data[i]; HAL_UartWriteByte(dump.data[i++]));
#endif
			}
			if (signal_status == true) {
				printStatus();
				signal_status = false;
			}
			c = messages_count();
			if (c == 0) {
				if (HAL_UartGetTxFifoBytes() == 0) {
					PHY_SetRxState(true);
					current_state = STATE_RECEIVING;
				}
			}
		}
	}
}

#endif

void APP_setupNetwork(void) {
#ifdef TEST_SENDER
	uint8_t b;
	uint16_t short_addr;
	b = boot_signature_byte_get(0x0102); short_addr = b;
	b = boot_signature_byte_get(0x0103); short_addr |= ((uint16_t)b << 8);
	NWK_SetAddr(short_addr);
#else
	NWK_SetAddr(0x0000);
#endif

	NWK_SetPanId(0x1973);
	PHY_SetChannel(0x16);
	PHY_SetTxPower(0);
	PHY_SetRxState(true);
}

void HAL_UartBytesReceived(uint16_t bytes) {

}

int main(void) {
	SYS_Init();
	HAL_LedInit();
	HAL_UartInit(38400);
	APP_setupNetwork();
	APP_setup();

	while (1) {
		SYS_TaskHandler();
		HAL_UartTaskHandler();
		APP_TaskHandler();
	}
}

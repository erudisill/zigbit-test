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
#define RECEIVER_DEQUEUE_PER_TICK 1
#define RECEIVER_MAX_IN_QUEUE_BEFORE_DUMP 1
#define RECEIVER_DUMP_TO_USART
//#define RECEIVER_TICKS_RECEIVING
#define RECEIVER_TICKS_DUMPING
//#define RECEIVER_STATUS_AS_STRING
#define RECEIVER_STATUS_AS_COBS

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

typedef struct {
	uint16_t header;
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
	uint16_t queue_count;
	uint16_t recs_per_sec;
} status_t;

status_t status;

uint8_t checksum;
bool signal_status;
char buffer[128];
uint8_t cobs_buffer[128];
message_t message;
enum {
	STATE_RECEIVING, STATE_DUMPING
};
uint8_t current_state;

size_t cobsEncode(uint8_t *input, uint8_t length, uint8_t *output) {
	size_t read_index = 0;
	size_t write_index = 1;
	size_t code_index = 0;
	uint8_t code = 1;

	while (read_index < length) {
		if (input[read_index] == 0) {
			output[code_index] = code;
			code = 1;
			code_index = write_index++;
			read_index++;
		} else {
			output[write_index++] = input[read_index++];
			code++;
			if (code == 0xFF) {
				output[code_index] = code;
				code = 1;
				code_index = write_index++;
			}
		}
	}

	output[code_index] = code;

	return write_index;
}

void printStatus(void) {

	status.header = 0xABAB;

	status.received_delta = status.received_count - status.received_count_prev;
	status.received_count_prev = status.received_count;

	status.error_delta = status.error_count - status.error_count_prev;
	status.error_count_prev = status.error_count;

	status.error_queue_delta = status.error_queue_count - status.error_queue_count_prev;
	status.error_queue_count_prev = status.error_queue_count;

	status.ticks_count_delta = status.ticks_count - status.ticks_count_prev;
	status.ticks_count_prev = status.ticks_count;

	status.queue_count = messages_count();
	status.recs_per_sec = status.received_count / status.secs_count;

#ifdef RECEIVER_STATUS_AS_STRING
	sprintf(buffer,
			"\r\n%lu\tR:%lu (+%d)\tRps:%d\tE:%lu (+%d)\tQ:%d\tQe:%lu (+%d)\tD:%d\tB:%lu\tT:%d\t",
			status.secs_count, status.received_count, status.received_delta, status.recs_per_sec,
			status.error_count, status.error_delta, status.queue_count, status.error_queue_count,
			status.error_queue_delta, status.dequeue_count, status.received_bytes,
			status.ticks_count_delta);
	for (int i = 0; buffer[i]; HAL_UartWriteByte(buffer[i++]))
		;
#endif

#ifdef RECEIVER_STATUS_AS_COBS
	uint8_t length = cobsEncode((uint8_t*)&status, sizeof(status_t), cobs_buffer);
	for (int i = 0; i < length; HAL_UartWriteByte(cobs_buffer[i++]))
		;
	HAL_UartWriteByte(0);
#endif

	status.dequeue_count = 0;
}

void statusTimerHandler(SYS_Timer_t *timer) {

	//printStatus();
	status.secs_count += 1;
	signal_status = true;
	(void) timer;
}

#ifdef MESSAGES_TEST
bool isValidMessage(NWK_DataInd_t *ind) {
	if (ind->size != sizeof(msg)) {
		return false;
	} else {
		uint8_t cs = 0;
		for (int i = 0; i < ind->size; cs ^= ind->data[i++])
		;
		if (cs != checksum) {
			return false;
		}
	}
	return true;
}
#endif
#ifdef MESSAGES_APPMSG
bool isValidMessage(NWK_DataInd_t *ind) {

	if (ind->size != sizeof(message_t)) {
		return false;
	}
	//TODO: Check for signature bytes
	return true;
}
#endif

bool appDataInd(NWK_DataInd_t *ind) {
	bool result;

	HAL_LedOn(LED_DATA);

	status.received_count += 1;
	status.received_bytes += ind->size;

	if (isValidMessage(ind) == true) {
#ifdef RECEIVER_USE_QUEUE
		memset(&message, 0, sizeof(message_t));
		memcpy(&message, ind->data, ind->size);
		result = messages_enqueue(&message);
		if (result == false) {
			status.error_queue_count += 1;
		}
#endif
	} else {
		status.error_count += 1;
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

	c = messages_count();
	if (c == 0 && signal_status == true) {
		printStatus();
		signal_status = false;
	}

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
			status.ticks_count += 1;
#endif
			if (c > RECEIVER_DEQUEUE_PER_TICK)
				c = RECEIVER_DEQUEUE_PER_TICK;

			for (int i = 0; i < c; i++) {
				memset(&dump, 0, sizeof(message_t));
				result = messages_dequeue(&dump);
				if (result == false)
					break;
				status.dequeue_count += 1;

#ifdef RECEIVER_DUMP_TO_USART
				uint8_t length = cobsEncode((uint8_t*)&dump, sizeof(message_t), cobs_buffer);
				for (int i = 0; i < length; HAL_UartWriteByte(cobs_buffer[i++]))
					;
				HAL_UartWriteByte(0);
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

/*
 * messages.h
 *
 *  Created on: Jan 16, 2015
 *      Author: ericrudisill
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

#include <stdbool.h>
#include <stdint.h>

//#define MESSAGES_TEST
#define MESSAGES_APPMSG


#define MESSAGE_QUEUE_SIZE 200

#ifdef MESSAGES_TEST
typedef struct
{
	char data[50];
} message_t;
#endif

#ifdef MESSAGES_APPMSG
typedef struct app_msg_t {
	uint8_t messageType;
	uint8_t nodeType;
	uint64_t extAddr;
	uint16_t shortAddr;
	uint64_t routerAddr;
	uint16_t panId;
	uint8_t workingChannel;
	uint16_t parentShortAddr;
	uint8_t lqi;
	int8_t rssi;
	uint8_t ackByte;
	int32_t battery;
	int32_t temperature;
	uint8_t cs;
} message_t;
#endif

bool messages_enqueue(message_t *);
bool messages_dequeue(message_t *);
bool messages_is_full(void);
bool messages_is_empty(void);
int messages_count(void);

#endif /* MESSAGES_H_ */

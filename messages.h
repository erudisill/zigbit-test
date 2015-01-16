/*
 * messages.h
 *
 *  Created on: Jan 16, 2015
 *      Author: ericrudisill
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

#include <stdbool.h>

#define MESSAGE_QUEUE_SIZE 100

typedef struct
{
	char data[50];
} message_t;

bool messages_enqueue(message_t *);
bool messages_dequeue(message_t *);
bool messages_is_full(void);
bool messages_is_empty(void);
int messages_count(void);

#endif /* MESSAGES_H_ */

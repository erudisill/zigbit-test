/*
 * messages.c
 *
 *  Created on: Jan 16, 2015
 *      Author: ericrudisill
 */

#include <stdbool.h>
#include <string.h>
#include "messages.h"

static message_t CQ[MESSAGE_QUEUE_SIZE];
static int f=-1,r=-1,c=0;

bool messages_enqueue(message_t * elem)
{
    if(messages_is_full())
    	return false;
    else
    {
        if(f==-1)
        	f=0;
        r=(r+1) % MESSAGE_QUEUE_SIZE;
        c+=1;
        memcpy(&CQ[r], elem, sizeof(message_t));
        return true;
    }
}

bool messages_dequeue(message_t * msg_dst)
{
    if(messages_is_empty())
    {
    	return false;
    }
    else
    {
    	memcpy(msg_dst, &CQ[f], sizeof(message_t));
        if(f==r)
        {
        	f=-1; r=-1, c=0;
        }
        else
        {
            f=(f+1) % MESSAGE_QUEUE_SIZE;
            c-=1;
        }
        return true;
    }
}

bool messages_is_full(void)
{
    if( (f==r+1) || (f == 0 && r== MESSAGE_QUEUE_SIZE-1))
    	return true;
    return false;
}

bool messages_is_empty(void)
{
    if(f== -1)
    	return true;
    return false;
}

int messages_count(void)
{
	return c;
}

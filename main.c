/**
 * \file main.c
 *
 * \brief zigbit-test
 *
 * Copyright (C) 2015 CP Handheld Technologies
 *
  */

#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/boot.h>
#include "config.h"
#include "hal.h"
#include "sys.h"
#include "halUart.h"
#include "halLed.h"

void HAL_UartBytesReceived(uint16_t bytes)
{

}

void APP_TaskHandler(void)
{

}

int main(void)
{
  SYS_Init();
  HAL_UartInit(38400);

  while (1)
  {
    SYS_TaskHandler();
    HAL_UartTaskHandler();
    APP_TaskHandler();
  }
}

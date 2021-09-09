/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

/* The source code comes from the VL53L1X real-time core driver of azure-sphere-gallery 
   https://github.com/Azure/azure-sphere-gallery/tree/main/BalancingRobot/Software/RTOS/rtos_app */ 

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

void delay(int ms);
void delayMicroseconds(int us);
void ListI2CDevices(int isuFd);

unsigned long millis(void);
void setmillis(unsigned long millis);
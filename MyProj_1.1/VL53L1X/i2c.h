/* The source code is modified from the VL53L1X real-time core driver of azure-sphere-gallery 
   https://github.com/Azure/azure-sphere-gallery/tree/main/BalancingRobot/Software/RTOS/rtos_app */ 

#include "applibs_versions.h"
#include "soc/mt3620_i2cs.h"
#include "applibs/i2c.h"
#include <applibs/log.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <hw/template_appliance.h> 

void InitI2C(void);

uint8_t writeByte(int _i2cfd, uint8_t deviceAddress, uint8_t registerAddress, uint8_t data);
uint8_t readByte(int _i2cfd, uint8_t deviceAddress, uint8_t registerAddress);
uint8_t readBytes(int _i2cfd, uint8_t deviceAddress, uint8_t registerAddress, uint8_t count, uint8_t* dest);
uint8_t writeBytes(int _i2cfd, uint8_t deviceAddress, uint8_t registerAddress, uint8_t* data, uint8_t count);

int GetI2CHandle(int isuNum);

// VL53L1X functions.
void writeRegister(int _i2cfd, uint16_t addr, uint8_t val);
void writeRegister16(int _i2cfd, uint16_t addr, uint16_t val);
void writeRegister32(int _i2cfd, uint16_t reg, uint32_t value);

uint8_t readRegister(int _i2cfd, uint16_t addr);
uint16_t readRegister16(int _i2cfd, uint16_t addr);
uint32_t readRegister32(int _i2cfd, uint16_t reg);

int readBlockData(int _i2cfd, uint8_t* txBuffer, uint8_t* rxBuffer, uint16_t writeLen, uint16_t readLen);


/* The source code is modified from the VL53L1X real-time core driver of azure-sphere-gallery 
   https://github.com/Azure/azure-sphere-gallery/tree/main/BalancingRobot/Software/RTOS/rtos_app */ 

#include "i2c.h"
#include <errno.h>
#include "utils.h"
#include <string.h>

static int _ISU0_fd = -1;
static int _ISU1_fd = -1;

static uint8_t writeBuffer[64];
static uint8_t readBuffer[64];

int Native_ReadData(int _i2cfd, uint8_t deviceAddress, uint8_t reg, void* data, size_t len);
int Native_ReadRegU8(int _i2cfd, uint8_t deviceAddress, uint8_t reg, uint8_t* value);
int Native_WriteData(int _i2cfd, uint8_t deviceAddress, uint8_t reg, void* data, size_t len);
int Native_WriteRegU8(int _i2cfd, uint8_t deviceAddress, uint8_t reg, uint8_t value);

uint8_t writeByteWire(int _i2cfd, uint8_t deviceAddress, uint8_t registerAddress, uint8_t data);
uint8_t readByteWire(int _i2cfd, uint8_t deviceAddress, uint8_t registerAddress);

// Wire.h read and write protocols
uint8_t writeByte(int _i2cfd, uint8_t deviceAddress, uint8_t registerAddress, uint8_t data)
{
    return writeByteWire(_i2cfd, deviceAddress, registerAddress, data);
}

uint8_t writeByteWire(int _i2cfd, uint8_t deviceAddress, uint8_t registerAddress,
    uint8_t data)
{
    Native_WriteRegU8(_i2cfd, deviceAddress, registerAddress, data);

    return NULL;
}

// Read a byte from given register on device. Calls necessary SPI or I2C
// implementation. This was configured in the constructor.
uint8_t readByte(int _i2cfd, uint8_t deviceAddress, uint8_t registerAddress)
{
    return readByteWire(_i2cfd, deviceAddress, registerAddress);
}

// Read a byte from the given register address from device using I2C
uint8_t readByteWire(int _i2cfd, uint8_t deviceAddress, uint8_t registerAddress)
{
    uint8_t data; // `data` will store the register data

    Native_ReadRegU8(_i2cfd, deviceAddress, registerAddress, &data);

    return data;
}

// Read 1 or more bytes from given register and device using I2C
uint8_t readBytesWire(int _i2cfd, uint8_t deviceAddress, uint8_t registerAddress,
    uint8_t count, uint8_t* dest)
{
    uint8_t i = Native_ReadData(_i2cfd, deviceAddress, registerAddress, dest, count);

    return i; // Return number of bytes written
}

uint8_t readBytes(int _i2cfd, uint8_t deviceAddress, uint8_t registerAddress, uint8_t count, uint8_t* dest)
{
    return readBytesWire(_i2cfd, deviceAddress, registerAddress, count, dest);
}

uint8_t writeBytes(int _i2cfd, uint8_t deviceAddress, uint8_t registerAddress, uint8_t* data, uint8_t count)
{
    return Native_WriteData(_i2cfd, deviceAddress, registerAddress, data, count);
}

void InitI2C(void)
{
    // if ISUs are already initialized, return.
    if (_ISU0_fd > -1 && _ISU1_fd > -1) 
        return;

    _ISU0_fd = I2CMaster_Open(I2C0); 
    if (_ISU0_fd == -1)
    {
        return;
    }

	Log_Debug("ISU0 - Handle %d\n", _ISU0_fd);

    int result = I2CMaster_SetBusSpeed(_ISU0_fd, I2C_BUS_SPEED_FAST);
    if (result != 0)
    {
        return;
    }

    result = I2CMaster_SetTimeout(_ISU0_fd, 100);
    if (result != 0) {
        return;
    }

   _ISU1_fd = I2CMaster_Open(I2C1); 
   if (_ISU1_fd == -1)
   {
       return;
	}

	Log_Debug("ISU1 - Handle %d\n", _ISU1_fd);

   result = I2CMaster_SetBusSpeed(_ISU1_fd, I2C_BUS_SPEED_FAST);
   if (result != 0)
   {
       return;
   }

   result = I2CMaster_SetTimeout(_ISU1_fd, 100);
   if (result != 0) {
       return;
   }

	Log_Debug("I2C0 init successfully.\n");
}

int Native_WriteRegU8(int _i2cfd, uint8_t deviceAddress, uint8_t reg, uint8_t value)
{
#ifdef SHOW_DEBUG_MSGS
    Log_Debug(">>> %s\n", __func__);
#endif
    uint8_t req[] = { (uint8_t)reg, value };
    int ret = Native_WriteData(_i2cfd, deviceAddress, reg, req, sizeof(req));
    return ret;
}

int Native_WriteData(int _i2cfd, uint8_t deviceAddress, uint8_t reg, void* data, size_t len)
{
#ifdef SHOW_DEBUG_MSGS
    Log_Debug(">>> %s\n", __func__);
#endif

    InitI2C();
    int ret = 0;
    int r = I2CMaster_Write(_i2cfd, deviceAddress, data, len);
    if (r != len) {
        Log_Debug("%s: I2CMaster_Write: errno=%d (%s)\n", __func__, errno, strerror(errno));
        ret = -1;
    }

    return ret;
}

int Native_ReadRegU8(int _i2cfd, uint8_t deviceAddress, uint8_t reg, uint8_t* value)
{
#ifdef SHOW_DEBUG_MSGS
    Log_Debug(">>> %s\n", __func__);
#endif

    int ret = Native_ReadData(_i2cfd, deviceAddress, reg, value, sizeof(*value));
    return ret;
}

int Native_ReadData(int _i2cfd, uint8_t deviceAddress, uint8_t reg, void* data, size_t len)
{
#ifdef SHOW_DEBUG_MSGS
    Log_Debug(">>> %s\n", __func__);
#endif

    InitI2C();

    int ret = 0;

    int retryCount = 0;
    while (true)
    {
        uint8_t reg8 = (uint8_t)reg;
        int r = I2CMaster_WriteThenRead(_i2cfd, deviceAddress, &reg8, sizeof(reg8), data, len);

        size_t expectedBytes = sizeof(reg8) + len;
        if (r != expectedBytes) {
            delay(1);
            retryCount++;
            if (retryCount > 5)
            {
                ret = -1;
                Log_Debug("Retry failed I2CMaster_WriteThenRead\n");
                break;
            }
        }
        else
        {
            ret = r;
            // expected result is ok.
            break;
        }
    }

    return ret;
}

int GetI2CHandle(int isuNum)
{
    InitI2C();
    if (isuNum == MT3620_I2C_ISU0)
    {
        return _ISU0_fd;
    }

    if (isuNum == MT3620_I2C_ISU1)
    {
        return _ISU1_fd;
    }

    return -1;
}


// VL53L1X Functions

//Write a byte to a spot
void writeRegister(int _i2cBus, uint16_t addr, uint8_t val)
{
	uint8_t buffer[3] = { 0 };
	buffer[0] = addr >> 8;
	buffer[1] = addr & 0xff;
	buffer[2] = val;

	__builtin_memcpy(writeBuffer, buffer, 3);

	int result = I2CMaster_Write(_i2cBus, 0x29, writeBuffer, 3);
	// int fnRes = mtk_os_hal_i2c_write(OS_HAL_I2C_ISU1, 0x29, writeBuffer, 3);
	if (result < 0)
	{
		Log_Debug("writeRegister failed\r\n");
		Log_Debug("ERROR: writeRegister: errno=%d (%s)\n", errno, strerror(errno));
	}
}

//Write two bytes to a spot
void writeRegister16(int _i2cBus, uint16_t addr, uint16_t val)
{
	uint8_t buffer[4] = { 0 };
	buffer[0] = addr >> 8;
	buffer[1] = addr & 0xff;
	buffer[2] = val >> 8;
	buffer[3] = val & 0xff;

	memcpy(writeBuffer, buffer, 4);

	int result = I2CMaster_Write(_i2cBus, 0x29, writeBuffer, 4);
	// int fnRes = mtk_os_hal_i2c_write(OS_HAL_I2C_ISU1, 0x29, writeBuffer, 4);
	if (result < 0)
	{
		Log_Debug("writeRegister16 failed\r\n");
		Log_Debug("ERROR: writeRegister16: errno=%d (%s)\n", errno, strerror(errno));
	}
}

void writeRegister32(int _i2cBus, uint16_t reg, uint32_t value)
{
	uint8_t buffer[6];

	buffer[0] = reg >> 8;
	buffer[1] = reg & 0xff;
	buffer[2] = (value >> 24) & 0xFF; // value highest byte
	buffer[3] = (value >> 16) & 0xFF;
	buffer[4] = (value >> 8) & 0xFF;
	buffer[5] = value & 0xFF; // value lowest byte

	memcpy(writeBuffer, buffer, 6);

	int result = I2CMaster_Write(_i2cBus, 0x29, writeBuffer, 6);
	// int fnRes = mtk_os_hal_i2c_write(OS_HAL_I2C_ISU1, 0x29, writeBuffer, 6);
	if (result < 0)
	{
		Log_Debug("writeRegister32 failed\r\n");
		Log_Debug("ERROR: writeRegister32: errno=%d (%s)\n", errno, strerror(errno));
		
	}
}

//Reads one byte from a given location
//Returns zero on error
uint8_t readRegister(int _i2cBus, uint16_t addr)
{
	uint8_t buffer[4] = { 0 };
	buffer[0] = addr >> 8;
	buffer[1] = addr & 0xff;

	memcpy(writeBuffer, buffer, 2);

	int result = I2CMaster_WriteThenRead(_i2cBus, 0x29, writeBuffer, 2, readBuffer, 1);
	// int result = mtk_os_hal_i2c_write_read(OS_HAL_I2C_ISU1, 0x29, writeBuffer, readBuffer, 2, 1);

	if (result < 0)
	{
		Log_Debug("readRegister - Failed I2CMaster_WriteThenRead (%d)\r\n", result);
		Log_Debug("ERROR: readRegister: errno=%d (%s)\n", errno, strerror(errno)); 
	}

	uint8_t value = 0;

	value = readBuffer[0];       // value lowest byte

	return value;
}

//Reads two consecutive bytes from a given location
//Returns zero on error
uint16_t readRegister16(int _i2cBus, uint16_t addr)
{
	uint8_t buffer[4] = { 0 };
	buffer[0] = addr >> 8;
	buffer[1] = addr & 0xff;

	memcpy(writeBuffer, buffer, 2);

	int result = I2CMaster_WriteThenRead(_i2cBus, 0x29, writeBuffer, 2, readBuffer, 2);
	// int result = mtk_os_hal_i2c_write_read(OS_HAL_I2C_ISU1, 0x29, writeBuffer, readBuffer, 2, 2);

	if (result < 0)
	{
		Log_Debug("readRegister16 - Failed I2CMaster_WriteThenRead (%d)\r\n", result);
		Log_Debug("ERROR: readRegister16: errno=%d (%s)\n", errno, strerror(errno)); 
	}

	uint16_t value = 0;

	value |= (uint16_t)readBuffer[0] << 8;
	value |= readBuffer[1];       // value lowest byte

	return value;
}

// Read a 32-bit register
uint32_t readRegister32(int _i2cBus, uint16_t reg)
{
	uint8_t buffer[4] = { 0 };
	buffer[0] = reg >> 8;
	buffer[1] = reg & 0xff;

	memcpy(writeBuffer, buffer, 2);

	int result = I2CMaster_WriteThenRead(_i2cBus, 0x29, writeBuffer, 2, readBuffer, 4);
	// int result = mtk_os_hal_i2c_write_read(OS_HAL_I2C_ISU1, 0x29, writeBuffer, readBuffer, 2, 4);

	if (result < 0)
	{
		Log_Debug("readRegister32 - Failed I2CMaster_WriteThenRead (%d)\r\n", result);
		Log_Debug("ERROR: readRegister32: errno=%d (%s)\n", errno, strerror(errno));
	}

	uint32_t value;

	value = (uint32_t)readBuffer[0] << 24; // value highest byte
	value |= (uint32_t)readBuffer[1] << 16;
	value |= (uint16_t)readBuffer[2] << 8;
	value |= readBuffer[3];       // value lowest byte

	return value;
}

int readBlockData(int _i2cBus, uint8_t* txBuffer, uint8_t* rxBuffer, uint16_t writeLen, uint16_t readLen)
{
	memcpy(writeBuffer, txBuffer, writeLen);
	int result = I2CMaster_WriteThenRead(_i2cBus, 0x29, writeBuffer, writeLen, readBuffer, readLen);
	memcpy(rxBuffer, readBuffer, readLen);

	if (result < 0)
	{
		Log_Debug("readRegister32 - Failed I2CMaster_WriteThenRead (%d)\r\n", result);
		Log_Debug("ERROR: readBlockData: errno=%d (%s)\n", errno, strerror(errno));
	}

	//int result = mtk_os_hal_i2c_write(OS_HAL_I2C_ISU1, 0x29, writeBuffer, writeLen);
	//if (result < 0)
	//{
	//	printf("readBlockData - write failed\r\n");
	//}

	//for (int x = 0; x < readLen; x++)
	//{
	//	result = mtk_os_hal_i2c_read(OS_HAL_I2C_ISU1, 0x29, readBuffer, 1);
	//	if (result < 0)
	//	{
	//		printf("readBlockData - read failed\r\n");
	//		break;
	//	}
	//	else
	//	{
	//		rxBuffer[x] = readBuffer[0];
	//	}
	//}

	// int result = mtk_os_hal_i2c_write_read(OS_HAL_I2C_ISU1, 0x29, writeBuffer, readBuffer, writeLen, readLen);
	// __builtin_memcpy(rxBuffer, readBuffer, readLen);

	return result;
}
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/i2c.h>

#include "lib_mlx90614.h"
#include "mlx90614_support.h"

/*******************************************************************************
* Forward declarations of private functions
*******************************************************************************/

static ssize_t
i2c_read(mlx90614_t *p_mlx, uint8_t reg_addr, uint8_t *p_data,
    uint32_t data_len);

static ssize_t
i2c_write(mlx90614_t *p_mlx, uint8_t reg_addr, const uint8_t *p_data,
    uint32_t data_len);

static uint8_t
crc8(uint8_t prev_crc, uint8_t data);

/*******************************************************************************
* Global variables
*******************************************************************************/

/*******************************************************************************
* Public function definitions
*******************************************************************************/

int
log_printf(const char *p_format, ...)
{
    va_list args;

    va_start(args, p_format);
    int result = Log_DebugVarArgs(p_format, args);
    va_end(args);

    return result;
}

bool
reg_read(mlx90614_t *p_mlx, uint8_t reg_addr, uint16_t *p_reg_value)
{
    // 2 byte register data is followed by 1 byte PEC - Packet Error Code
    // The PEC calculation includes all bits except the START, REPEATED START, 
    // STOP, ACK, and NACK bits. The PEC is a CRC - 8 with polynomial
    // X8 + X2 + X1 + 1. The Most Significant Bit of every byte is transferred 
    // first.

    bool b_result = false;
    uint8_t buffer[3];  // LSB, MSB, PEC

    if (i2c_read(p_mlx, reg_addr, buffer, 3) != -1)
    {
        uint8_t crc = crc8(0, (uint8_t)(p_mlx->i2c_addr << 1));
        crc = crc8(crc, reg_addr);
        crc = crc8(crc, (uint8_t)(p_mlx->i2c_addr << 1) | 1);
        crc = crc8(crc, buffer[0]);
        crc = crc8(crc, buffer[1]);

        if (buffer[2] == crc)       // PEC
        {
            *p_reg_value = (uint16_t)((buffer[1] << 8) | buffer[0]);
            b_result = true;
        }
    }

    return b_result;
}

bool
reg_write(mlx90614_t *p_mlx, uint8_t reg_addr, uint16_t reg_value)
{
    bool b_result = false;
    uint8_t buffer[3];  // LSB, MSB, CRC

    buffer[0] = (uint8_t)(reg_value & 0x00FF);
    buffer[1] = (uint8_t)(reg_value >> 8);

    buffer[2] = crc8(0, (uint8_t)(p_mlx->i2c_addr << 1));
    buffer[2] = crc8(buffer[2], reg_addr);
    buffer[2] = crc8(buffer[2], buffer[0]);
    buffer[2] = crc8(buffer[2], buffer[1]);

    if (i2c_write(p_mlx, reg_addr, buffer, 3) != -1)
    {
        b_result = true;
    }

    return b_result;
}

bool
eeprom_write(mlx90614_t *p_mlx, uint8_t reg_addr, uint16_t reg_value)
{
    // Note: A write of 0x0000 must be done prior to writing in EEPROM in order 
    // to erase the EEPROM cell content

    struct timespec delay_time;
    delay_time.tv_sec = 0;

    bool b_result = reg_write(p_mlx, reg_addr, 0);
    delay_time.tv_nsec = MLX90614_T_ERASE_MS * 1000000;
    nanosleep(&delay_time, NULL);   // Wait for EEPROM to erase

    if (b_result)
    {
        b_result = reg_write(p_mlx, reg_addr, reg_value);
        delay_time.tv_nsec = MLX90614_T_WRITE_MS * 1000000;
        nanosleep(&delay_time, NULL);   // Wait for EEPROM to write new value
    }

    return b_result;
}

/*******************************************************************************
* Private function definitions
*******************************************************************************/

static ssize_t
i2c_read(mlx90614_t *p_mlx, uint8_t reg_addr, uint8_t *p_data, 
    uint32_t data_len)
{
    ssize_t result = -1;

    if (p_mlx && p_data)
    {
#   	ifdef MLX90614_I2C_DEBUG
        DEBUG_DEV(" REG READ [%02X] bytes %d", __FUNCTION__, p_mlx, reg_addr,
            data_len);
#       endif

        // Select register and read its data
        result = I2CMaster_WriteThenRead(p_mlx->i2c_fd, p_mlx->i2c_addr,
            &reg_addr, 1, p_data, data_len);

        if (result == -1)
        {
#   	    ifdef MLX90614_I2C_DEBUG
            DEBUG_DEV("Error %d (%s) on I2C WR operation at addr 0x%02X",
                __FUNCTION__, p_mlx, errno, strerror(errno), p_mlx->i2c_addr);
#           endif
        }
        else
        {
#   	ifdef MLX90614_I2C_DEBUG
            log_printf("MLX %s (0x%02X):  READ ", __FUNCTION__, p_mlx->i2c_addr);
            for (int i = 0; i < data_len; i++)
            {
                log_printf("%02X ", p_data[i]);
            }
            log_printf("\n");
#           endif
        }
    }

    // Return length of read data only
    return result - 1;
}

static ssize_t
i2c_write(mlx90614_t *p_mlx, uint8_t reg_addr, const uint8_t *p_data,
    uint32_t data_len)
{
    ssize_t result = -1;

    if (p_mlx && p_data)
    {
#   	ifdef MLX90614_I2C_DEBUG
        DEBUG_DEV(" REG WRITE [%02X] bytes %d", __FUNCTION__, p_mlx, reg_addr,
            data_len);
#       endif

        uint8_t buffer[data_len + 1];

        buffer[0] = reg_addr;
        for (uint32_t i = 0; i < data_len; i++)
        {
            buffer[i + 1] = p_data[i];
        }

#		ifdef MLX90614_I2C_DEBUG
        log_printf("MLX %s (0x%02X):  WRITE ", __FUNCTION__, p_mlx->i2c_addr);
        for (int i = 0; i < data_len; i++)
        {
            log_printf("%02X ", p_data[i]);
        }
        log_printf("\n");
#		endif

        // Select register and write data
        result = I2CMaster_Write(p_mlx->i2c_fd, p_mlx->i2c_addr, buffer,
            data_len + 1);

        if (result == -1)
        {
#		    ifdef MLX90614_I2C_DEBUG
            DEBUG_DEV("Error %d (%s) on writing %d byte(s) to I2C addr 0x%02X",
                __FUNCTION__, p_mlx, errno, strerror(errno), data_len + 1,
                p_mlx->i2c_addr);
#           endif
        }
    }

    return result;
}

static uint8_t
crc8(uint8_t prev_crc, uint8_t data)
{
    uint8_t result = prev_crc ^ data;
    for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++)
    {
        if ((result & 0x80) != 0)
        {
            result = (uint8_t)(result << 1);
            result = result ^ 0x07;
        }
        else
        {
            result = (uint8_t)(result << 1);
        }
    }
    return result;
}


/* [] END OF FILE */

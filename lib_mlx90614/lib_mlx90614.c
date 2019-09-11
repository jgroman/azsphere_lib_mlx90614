/***************************************************************************//**
* @file    lib_mlx90614.c
* @version 1.0.0
*
* @brief MLX90614 IR sensor support for Azure Sphere.
*
* Code ported from https://github.com/sparkfun/SparkFun_MLX90614_Arduino_Library
* by Jim Lindblom @ SparkFun Electronics
*
* @author   Jaroslav Groman
*
* @date
*
*******************************************************************************/

#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <applibs/log.h>
#include <applibs/i2c.h>

#include "lib_mlx90614.h"
#include "mlx90614_support.h"

/*******************************************************************************
* Forward declarations of private functions
*******************************************************************************/

bool
get_tobj1(mlx90614_t *p_mlx);

bool
get_tobj2(mlx90614_t *p_mlx);

bool
get_ta(mlx90614_t *p_mlx);

bool
get_tomax(mlx90614_t *p_mlx);

bool
get_tomin(mlx90614_t *p_mlx);

int16_t
convert_temp_unit_to_linear(mlx90614_t *p_mlx, float united_temp);

float
convert_temp_linear_to_unit(mlx90614_t *p_mlx, int16_t linear_temp);


/*******************************************************************************
* Global variables
*******************************************************************************/


/*******************************************************************************
* Function definitions
*******************************************************************************/

mlx90614_t
*mlx90614_open(int i2c_fd, I2C_DeviceAddress i2c_addr)
{
    mlx90614_t *p_mlx = NULL;
    bool b_is_init_ok = true;

    if ((p_mlx = malloc(sizeof(mlx90614_t))) == NULL)
    {
        // Cannot allocate memory for device descriptor
        b_is_init_ok = false;
        ERROR("Not enough free memory.", __FUNCTION__);
    }

    // Initialize device descriptor, check device ID
    if (b_is_init_ok)
    {
        p_mlx->i2c_fd = i2c_fd;
        p_mlx->i2c_addr = i2c_addr;

        p_mlx->temperature_unit = MLX_TEMP_CELSIUS;

        // Read device ID
        DEBUG_DEV("--- Reading sensor ID", __FUNCTION__, p_mlx);

        mlx90614_read_id(p_mlx);

    }

    if (!b_is_init_ok)
    {
        ERROR("MLX90614 initialization failed.", __FUNCTION__);
        free(p_mlx);
        p_mlx = NULL;
    }

    return p_mlx;
}

void
mlx90614_close(mlx90614_t *p_mlx)
{
    // Free memory allocated to device decriptor
    free(p_mlx);
}

void
mlx90614_set_temperature_unit(mlx90614_t *p_mlx, mlx_temperature_unit unit)
{
    p_mlx->temperature_unit = unit;
}


bool
mlx90614_get_id(mlx90614_t *p_mlx)
{
    int16_t reg_value;
    bool b_result = false;

    for (register uint8_t idx = 0; idx < 4; idx++)
    {
        b_result = reg_read(p_mlx, (uint8_t)(MLX90614_EREG_ID1 + idx), 
            &reg_value);

        if (!b_result)
        {
            break;
        }

        p_mlx->device_id[idx] = (uint16_t)reg_value;
    }

    return b_result;
}


bool
mlx90614_get_emissivity(mlx90614_t *p_mlx, float *p_emmisivity)
{
    int16_t ecc;

    bool b_result = reg_read(p_mlx, MLX90614_EREG_ECC, &ecc);

    if (b_result)
    {
        *p_emmisivity = (float)ecc / 65535.0;
    }
    return b_result;
}

bool
mlx90614_set_emissivity(mlx90614_t *p_mlx, float emissivity)
{
    bool b_result = false;

    if ((emissivity >= 0.1) && (emissivity <= 1.0))
    {
        uint16_t ecc = (uint16_t)(emissivity * 65535.0);

        if (ecc < 0x2000)
        {
            ecc = 0x2000;
        }

        b_result = eeprom_write(p_mlx, MLX90614_EREG_ECC, (int16_t)ecc);
    }

    return b_result;
}

bool
mlx90614_get_address(mlx90614_t *p_mlx, uint8_t *p_address)
{
    int16_t addr;

    bool b_result = reg_read(p_mlx, MLX90614_EREG_SMBUS_ADDR, &addr);

    if (b_result)
    {
        *p_address = (uint8_t) addr & 0xFF;
    }
    return b_result;
}

bool
mlx90614_set_address(mlx90614_t *p_mlx, uint8_t address)
{
    bool b_result = false;

    if ((address > 0x00) && (address < 0x80))
    {
        uint16_t temp_addr;

        b_result = reg_read(p_mlx, MLX90614_EREG_SMBUS_ADDR, &temp_addr);

        if (b_result)
        {
            temp_addr &= 0xFF00;    // Keep MSB, clear LSB
            temp_addr |= address;   // Set LSB to address

            b_result = eeprom_write(p_mlx, MLX90614_EREG_SMBUS_ADDR, temp_addr);
        }
    }
    else
    {
        ERROR("I2C Address not set: address out of range.", __FUNCTION__);
    }

    return b_result;
}

bool
mlx90614_set_tomax(mlx90614_t *p_mlx, float temp_max)
{
    int16_t linear_max = convert_temp_unit_to_linear(p_mlx, temp_max);

    return eeprom_write(p_mlx, MLX90614_EREG_TOMAX, linear_max);
}

bool
mlx90614_set_tomin(mlx90614_t *p_mlx, float temp_min)
{
    int16_t linear_min = convert_temp_unit_to_linear(p_mlx, temp_min);

    return eeprom_write(p_mlx, MLX90614_EREG_TOMIN, linear_min);
}

bool
mlx90614_read_temperatures(mlx90614_t *p_mlx)
{
    bool b_result = false;

    if (get_tobj1(p_mlx) && get_tobj2(p_mlx) && get_ta(p_mlx))
    {
        b_result = true;
    }

    return b_result;
}

bool
mlx90614_read_range(mlx90614_t *p_mlx)
{
    bool b_result = false;

    if (get_tomax(p_mlx) && get_tomin(p_mlx) && get_ta(p_mlx))
    {
        b_result = true;
    }

    return b_result;
}


/*******************************************************************************
* Private function definitions
*******************************************************************************/

bool
get_tobj1(mlx90614_t *p_mlx)
{
    int16_t tobj1;

    bool b_result = reg_read(p_mlx, MLX90614_RREG_TOBJ1, &tobj1);

    if (b_result)
    {
        if (tobj1 & 0x8000)
        {
            b_result = false;
        }
        else
        {
            p_mlx->tobj1 = tobj1;
        }
    }

    return b_result;
}

bool
get_tobj2(mlx90614_t *p_mlx)
{
    int16_t tobj2;

    bool b_result = reg_read(p_mlx, MLX90614_RREG_TOBJ2, &tobj2);

    if (b_result)
    {
        if (tobj2 & 0x8000)
        {
            b_result = false;
        }
        else
        {
            p_mlx->tobj2 = tobj2;
        }
    }

    return b_result;
}

bool
get_ta(mlx90614_t *p_mlx)
{
    int16_t ta;

    bool b_result = reg_read(p_mlx, MLX90614_RREG_TA, &ta);

    if (b_result)
    {
        p_mlx->tobj2 = ta;
    }

    return b_result;
}

bool
get_tomax(mlx90614_t *p_mlx)
{
    int16_t tomax;

    bool b_result = reg_read(p_mlx, MLX90614_EREG_TOMAX, &tomax);

    if (b_result)
    {
        p_mlx->tomax = tomax;
    }

    return b_result;
}

bool
get_tomin(mlx90614_t *p_mlx)
{
    int16_t tomin;

    bool b_result = reg_read(p_mlx, MLX90614_EREG_TOMIN, &tomin);

    if (b_result)
    {
        p_mlx->tomin = tomin;
    }

    return b_result;
}

bool
get_ta_range(mlx90614_t *p_mlx)
{
    uint16_t ta_range;

    bool b_result = reg_read(p_mlx, MLX90614_EREG_TA_RANGE, &ta_range);

    if (b_result)
    {
        p_mlx->ta_range = ta_range;
    }

    return b_result;
}


int16_t
convert_temp_unit_to_linear(mlx90614_t *p_mlx, float united_temp)
{
    int16_t linear_temp;
    float kelvin_temp;

    if (p_mlx->temperature_unit == MLX_TEMP_LINEARIZED)
    {
        linear_temp = (int16_t)united_temp;
    }
    else
    {
        if (p_mlx->temperature_unit == MLX_TEMP_FAHRENHEIT)
        {
            kelvin_temp = (united_temp - 32.0) * 5.0 / 9.0 + 273.15;
        }
        else if (p_mlx->temperature_unit == MLX_TEMP_CELSIUS)
        {
            kelvin_temp = united_temp + 273.15;
        }
        else  // p_mlx->temperature_unit == MLX_TEMP_KELVIN
        {
            kelvin_temp = united_temp;
        }

        linear_temp = (int16_t)(kelvin_temp * 50); // Multiply by 0.02 degK/bit
    }

    return linear_temp;
}

float
convert_temp_linear_to_unit(mlx90614_t *p_mlx, int16_t linear_temp)
{
    float united_temp;

    if (p_mlx->temperature_unit == MLX_TEMP_LINEARIZED)
    {
        united_temp = (float)linear_temp;
    }
    else
    {
        united_temp = (float)linear_temp * 0.02;

        if (p_mlx->temperature_unit != MLX_TEMP_KELVIN)
        {
            united_temp -= 273.15;

            if (p_mlx->temperature_unit == MLX_TEMP_FAHRENHEIT)
            {
                united_temp = united_temp * 9.0 / 5.0 + 32;
            }
        }
    }

    return united_temp;
}

/* [] END OF FILE */

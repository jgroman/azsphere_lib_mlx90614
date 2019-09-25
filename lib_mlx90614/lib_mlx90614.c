/***************************************************************************//**
* @file    lib_mlx90614.c
* @version 1.0.0
*
* @brief MLX90614 IR sensor support for Azure Sphere.
*
* Code ported and modified 
* from https://github.com/sparkfun/SparkFun_MLX90614_Arduino_Library
* by Jim Lindblom @ SparkFun Electronics
*
* @author   Jaroslav Groman
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

/**
 * @brief Convert temperature from units to linearized value.
 *
 * @param united_temp Temperature value in units.
 * @param unit Temperature unit.
 *
 * @return Linearized temperature value.
 */
static int16_t
convert_temp_unit_to_linear(float united_temp, mlx_temperature_unit unit);

/**
 * @brief Convert temperature from linearized value to units.
 *
 * @param united_temp Linearized temperature value.
 * @param unit Temperature unit.
 *
 * @return Temperature value in units.
 */
static float
convert_temp_linear_to_unit(int16_t linear_temp, mlx_temperature_unit unit);

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
        b_is_init_ok = mlx90614_get_id(p_mlx);
    }

    if (!b_is_init_ok)
    {
        ERROR("MLX90614 initialization failed.", __FUNCTION__);
        if (p_mlx)
        {
            free(p_mlx);
            p_mlx = NULL;
        }
    }

    return p_mlx;
}

void
mlx90614_close(mlx90614_t *p_mlx)
{
    // Free memory allocated to device decriptor
    if (p_mlx)
    {
        free(p_mlx);
        p_mlx = NULL;
    }
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
        b_result = mlx90614_reg_read(p_mlx, (uint8_t)(MLX90614_EREG_ID1 + idx),
            &reg_value);

        if (!b_result)
        {
            break;
        }

        p_mlx->device_id[idx] = (uint16_t)reg_value;
    }

    return b_result;
}

I2C_DeviceAddress
mlx90614_get_address(mlx90614_t *p_mlx)
{
    int16_t addr;
    I2C_DeviceAddress result = 0;

    if (mlx90614_reg_read(p_mlx, MLX90614_EREG_SMBUS_ADDR, &addr))
    {
        result = (I2C_DeviceAddress) addr & 0xFF;
    }
    return result;
}

bool
mlx90614_set_address(mlx90614_t *p_mlx, I2C_DeviceAddress address)
{
    bool b_result = false;

    if ((address > 0x00) && (address < 0x80))
    {
        uint16_t temp_addr;

        b_result = mlx90614_reg_read(p_mlx, MLX90614_EREG_SMBUS_ADDR, 
            &temp_addr);

        if (b_result)
        {
            temp_addr &= 0xFF00;    // Keep MSB, clear LSB

            // Set LSB to address
            temp_addr = temp_addr | (uint16_t) address;

            b_result = mlx90614_eeprom_write(p_mlx, MLX90614_EREG_SMBUS_ADDR,
                (int16_t) temp_addr);
        }
    }
    else
    {
        ERROR("I2C Address not set: address out of range.", __FUNCTION__);
    }

    return b_result;
}

float
mlx90614_get_temperature_object1(mlx90614_t *p_mlx)
{
    int16_t tobj1;
    float result = MLX90614_TEMP_ERROR;

    if (mlx90614_reg_read(p_mlx, MLX90614_RREG_TOBJ1, &tobj1))
    {
        if (tobj1 & 0x8000)
        {
            ERROR("Error flag set on object1 temperature.", __FUNCTION__);
        }
        else
        {
            result = convert_temp_linear_to_unit(tobj1, 
                p_mlx->temperature_unit);
        }
    }

    return result;
}

float
mlx90614_get_temperature_object2(mlx90614_t *p_mlx)
{
    int16_t tobj2;
    float result = MLX90614_TEMP_ERROR;

    if (mlx90614_reg_read(p_mlx, MLX90614_RREG_TOBJ2, &tobj2))
    {
        if (tobj2 & 0x8000)
        {
            ERROR("Error flag set on object2 temperature.", __FUNCTION__);
        }
        else
        {
            result = convert_temp_linear_to_unit(tobj2, 
                p_mlx->temperature_unit);
        }
    }

    return result;
}

float
mlx90614_get_temperature_ambient(mlx90614_t *p_mlx)
{
    int16_t ta;
    float result = MLX90614_TEMP_ERROR;

    if (mlx90614_reg_read(p_mlx, MLX90614_RREG_TA, &ta))
    {
        result = convert_temp_linear_to_unit(ta, p_mlx->temperature_unit);
    }

    return result;
}

float
mlx90614_get_emissivity(mlx90614_t *p_mlx)
{
    int16_t ecc;
    float result = MLX90614_EMISSIVITY_ERROR;

    if (mlx90614_reg_read(p_mlx, MLX90614_EREG_ECC, &ecc))
    {
        result = (float)ecc / 65535.0F;
    }
    return result;
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

        b_result = mlx90614_eeprom_write(p_mlx, MLX90614_EREG_ECC, (int16_t)ecc);
    }
    else
    {
        ERROR("Emissivity not set: value out of range.", __FUNCTION__);
    }

    return b_result;
}

float
mlx90614_get_tobj_range_min(mlx90614_t *p_mlx)
{
    int16_t tomin;
    float result = MLX90614_TEMP_ERROR;

    if (mlx90614_reg_read(p_mlx, MLX90614_EREG_TOMIN, &tomin))
    {
        result = convert_temp_linear_to_unit(tomin, p_mlx->temperature_unit);
    }

    return result;
}

bool
mlx90614_set_tobj_range_min(mlx90614_t *p_mlx, float t_min)
{
    int16_t linear_min = convert_temp_unit_to_linear(t_min,
        p_mlx->temperature_unit);
    return mlx90614_eeprom_write(p_mlx, MLX90614_EREG_TOMIN, linear_min);
}

float
mlx90614_get_tobj_range_max(mlx90614_t *p_mlx)
{
    int16_t tomax;
    float result = MLX90614_TEMP_ERROR;

    if (mlx90614_reg_read(p_mlx, MLX90614_EREG_TOMAX, &tomax))
    {
        result = convert_temp_linear_to_unit(tomax, p_mlx->temperature_unit);
    }

    return result;
}

bool
mlx90614_set_tobj_range_max(mlx90614_t *p_mlx, float t_max)
{
    int16_t linear_max = convert_temp_unit_to_linear(t_max, 
        p_mlx->temperature_unit);
    return mlx90614_eeprom_write(p_mlx, MLX90614_EREG_TOMAX, linear_max);
}

float
mlx90614_get_ta_range_min(mlx90614_t *p_mlx)
{
    uint16_t tarange;
    float result = MLX90614_TEMP_ERROR;

    if (mlx90614_reg_read(p_mlx, MLX90614_EREG_TA_RANGE, &tarange))
    {
        result = convert_temp_linear_to_unit((tarange & 0x00FF), 
            p_mlx->temperature_unit);
    }
    return result;
}

float
mlx90614_get_ta_range_max(mlx90614_t *p_mlx)
{
    uint16_t tarange;
    float result = MLX90614_TEMP_ERROR;

    if (mlx90614_reg_read(p_mlx, MLX90614_EREG_TA_RANGE, &tarange))
    {
        result = convert_temp_linear_to_unit((uint8_t)(tarange >> 8), 
            p_mlx->temperature_unit);
    }
    return result;
}

/*******************************************************************************
* Private function definitions
*******************************************************************************/

static int16_t
convert_temp_unit_to_linear(float united_temp, mlx_temperature_unit unit)
{
    int16_t linear_temp;
    float kelvin_temp;

    if (unit == MLX_TEMP_LINEARIZED)
    {
        linear_temp = (int16_t)united_temp;
    }
    else
    {
        if (unit == MLX_TEMP_FAHRENHEIT)
        {
            kelvin_temp = (united_temp - 32.0F) * 5.0F / 9.0F + 273.15F;
        }
        else if (unit == MLX_TEMP_CELSIUS)
        {
            kelvin_temp = united_temp + 273.15F;
        }
        else  // p_mlx->temperature_unit == MLX_TEMP_KELVIN
        {
            kelvin_temp = united_temp;
        }

        linear_temp = (int16_t)(kelvin_temp * 50); // Multiply by 0.02 degK/bit
    }

    return linear_temp;
}

static float
convert_temp_linear_to_unit(int16_t linear_temp, mlx_temperature_unit unit)
{
    float united_temp;

    if (unit == MLX_TEMP_LINEARIZED)
    {
        united_temp = (float)linear_temp;
    }
    else
    {
        united_temp = (float)linear_temp * 0.02F;

        if (unit != MLX_TEMP_KELVIN)
        {
            united_temp -= 273.15F;

            if (unit == MLX_TEMP_FAHRENHEIT)
            {
                united_temp = united_temp * 9.0F / 5.0F + 32;
            }
        }
    }

    return united_temp;
}

/* [] END OF FILE */

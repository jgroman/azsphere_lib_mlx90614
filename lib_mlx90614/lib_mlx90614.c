/***************************************************************************//**
* @file    lib_mlx90614.c
* @version 1.0.0
*
* @brief .
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

        // Read device ID
        DEBUG_DEV("--- Reading sensor ID", __FUNCTION__, p_mlx);

        uint16_t id[4];
        mlx90614_read_id(p_mlx, id);

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

bool
mlx90614_read_id(mlx90614_t *p_mlx)
{
    uint16_t reg_value;
    bool b_result = false;

    for (register uint8_t idx = 0; idx < 4; idx++)
    {
        b_result = reg_read(p_mlx, (uint8_t)(MLX90614_EREG_ID1 + idx), 
            &reg_value);

        if (!b_result)
        {
            break;
        }

        p_mlx->device_id[idx] = reg_value;
    }

    return b_result;
}

bool
mlx90614_read_tobj1(mlx90614_t *p_mlx)
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
mlx90614_read_tobj2(mlx90614_t *p_mlx)
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
mlx90614_read_ta(mlx90614_t *p_mlx)
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
mlx90614_read_tomax(mlx90614_t *p_mlx)
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
mlx90614_read_tomin(mlx90614_t *p_mlx)
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
mlx90614_read_ecc(mlx90614_t *p_mlx, float *p_emmisivity)
{
    int16_t ecc

    bool b_result = reg_read(p_mlx, MLX90614_EREG_ECC, &ecc);

    if (b_result)
    {
        *p_emmisivity = (float)ecc / 65535.0;
    }
    return b_result;
}

bool
mlx90614_read_address(mlx90614_t *p_mlx, uint8_t *p_address)
{
    uint16_t addr;

    bool b_result = reg_read(p_mlx, MLX90614_EREG_SMBUS_ADDR, &addr);

    if (b_result)
    {
        *p_address = (uint8_t) addr & 0xFF;
    }
    return b_result;
}


/*******************************************************************************
* Private function definitions
*******************************************************************************/



/* [] END OF FILE */

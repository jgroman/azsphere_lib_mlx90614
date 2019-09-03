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
mlx90614_read_id(mlx90614_t *p_mlx, uint16_t *p_arr_id)
{
    uint16_t reg_value;
    bool b_result = false;

    for (register uint8_t idx = 0; idx < 4; idx++)
    {
        b_result = reg_read(p_mlx, (uint8_t)(MLX90614_EREG_ID1 + idx), &reg_value);

        if (!b_result)
        {
            break;
        }

        p_arr_id[idx] = reg_value;
    }

    return b_result;
}

/*******************************************************************************
* Private function definitions
*******************************************************************************/



/* [] END OF FILE */

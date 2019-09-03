/***************************************************************************//**
* @file    main.c
* @version 1.0.0
*
* @brief .
*
* @author Jaroslav Groman
*
* @date
*
*******************************************************************************/

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "applibs_versions.h"   // API struct versions to use for applibs APIs
#include <applibs/log.h>
#include <applibs/i2c.h>

// Import project hardware abstraction from project property 
// "Target Hardware Definition Directory"
#include <hw/project_hardware.h>

#include "lib_mlx90614.h"

/*******************************************************************************
* Forward declarations of private functions
*******************************************************************************/

/**
 * @brief Application termination handler.
 *
 * Signal handler for termination requests. This handler must be
 * async-signal-safe.
 *
 * @param signal_number
 *
 */
static void
termination_handler(int signal_number);

/**
 * @brief Initialize signal handlers.
 *
 * Set up SIGTERM termination handler.
 *
 * @return 0 on success, -1 otherwise.
 */
static int
init_handlers(void);

/**
 * @brief Initialize peripherals.
 *
 * Initialize all peripherals used by this project.
 *
 * @return 0 on success, -1 otherwise.
 */
static int
init_peripherals(I2C_InterfaceId isu_id);

/**
 *
 */
static void
close_peripherals_and_handlers(void);

/*******************************************************************************
* Global variables
*******************************************************************************/

// Termination state flag
static volatile sig_atomic_t gb_is_termination_requested = false;

static int i2c_fd = -1;
static mlx90614_t *p_mlx;

/*******************************************************************************
* Function definitions
*******************************************************************************/

int
main(int argc, char *argv[])
{
    Log_Debug("\n*** Starting ***\n");

    gb_is_termination_requested = false;

    // Initialize handlers
    if (init_handlers() != 0)
    {
        gb_is_termination_requested = true;
    }

    // Initialize peripherals
    if (!gb_is_termination_requested)
    {
        if (init_peripherals(PROJECT_ISU2_I2C) != 0)
        {
            gb_is_termination_requested = true;
        }
    }

    // Main program
    if (!gb_is_termination_requested)
    {

        // Main program loop
        while (!gb_is_termination_requested)
        {
        }

        Log_Debug("Leaving main loop\n");

    }

    close_peripherals_and_handlers();

    Log_Debug("*** Terminating ***\n");
    return 0;

}

/*******************************************************************************
* Private function definitions
*******************************************************************************/

static void
termination_handler(int signal_number)
{
    gb_is_termination_requested = true;
}

static int
init_handlers(void)
{
    Log_Debug("Init Handlers\n");

    int result = -1;

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = termination_handler;
    result = sigaction(SIGTERM, &action, NULL);
    if (result != 0) {
        Log_Debug("ERROR: %s - sigaction: errno=%d (%s)\n",
            __FUNCTION__, errno, strerror(errno));
    }

    return result;
}

static int
init_peripherals(I2C_InterfaceId isu_id)
{
    int result = -1;

    // Initialize I2C
    Log_Debug("Init I2C\n");
    i2c_fd = I2CMaster_Open(isu_id);
    if (i2c_fd < 0) {
        Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n",
            errno, strerror(errno));
    }
    else
    {
        result = I2CMaster_SetBusSpeed(i2c_fd, I2C_BUS_SPEED_STANDARD);
        if (result != 0) {
            Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n",
                errno, strerror(errno));
        }
        else
        {
            result = I2CMaster_SetTimeout(i2c_fd, 100);
            if (result != 0) {
                Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n",
                    errno, strerror(errno));
            }
        }
    }

    // Initialize MLX90614 board
    if (result != -1)
    {
        Log_Debug("Init MLX90614\n");
        p_mlx = mlx90614_open(i2c_fd, MLX90614_I2C_ADDRESS);

        if (!p_mlx)
        {
            Log_Debug("ERROR: Could not initialize AMLX90614.\n");
            result = -1;
        }

    }

    return result;
}

static void
close_peripherals_and_handlers(void)
{
    // Close MLX90614 sensor
    Log_Debug("Close MLX90614\n");
    if (p_mlx)
    {
        mlx90614_close(p_mlx);
    }

    // Close I2C
    if (i2c_fd)
    {
        close(i2c_fd);
    }
}
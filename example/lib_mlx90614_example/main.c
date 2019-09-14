/***************************************************************************//**
* @file    main.c
* @version 1.0.0
*
* @brief Azure Sphere MLX90614 IR sensor usage example.
*
* @author Jaroslav Groman
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
#include <applibs/gpio.h>
#include <applibs/i2c.h>

// Import project hardware abstraction from project property 
// "Target Hardware Definition Directory"
#include <hw/project_hardware.h>

// Using a single-thread event loop pattern based on Epoll and timerfd
#include "epoll_timerfd_utilities.h"

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

/**
 * @brief Button1 press handler
 */
static void
button1_press_handler(void);

/**
 * @brief Timer event handler for polling button states
 */
static void
button_timer_event_handler(EventData *event_data);

/*******************************************************************************
* Global variables
*******************************************************************************/

// Termination state flag
static volatile sig_atomic_t gb_is_termination_requested = false;

static int i2c_fd = -1;
static int epoll_fd = -1;

static int button_poll_timer_fd = -1;
static int button1_gpio_fd = -1;
static GPIO_Value_Type button1_state = GPIO_Value_High;
static EventData button_event_data = {          // Event handler data
    .eventHandler = &button_timer_event_handler // Populate only this field
};

static mlx90614_t *p_mlx;   // MLX90614 sensor device descriptor pointer

/*******************************************************************************
* Function definitions
*******************************************************************************/

int
main(int argc, char *argv[])
{
    const struct timespec sleep_time = { 1, 0 };
    float temp1, tambient;

    Log_Debug("\n*** Starting ***\n");
    Log_Debug("Press Button 1 to exit.\n");

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

    // Set measurement unit to degrees Celsius
    mlx90614_set_temperature_unit(p_mlx, MLX_TEMP_CELSIUS);

    // Main program
    if (!gb_is_termination_requested)
    {
        Log_Debug("Waiting for timer events\n");

        // Main program loop
        while (!gb_is_termination_requested)
        {

            // Handle timers
            if (WaitForEventAndCallHandler(epoll_fd) != 0)
            {
                gb_is_termination_requested = true;
            }

            temp1 = mlx90614_get_temperature_object1(p_mlx);
            tambient = mlx90614_get_temperature_ambient(p_mlx);

            Log_Debug("Temperatures: To1 %.1f, Ta %.1f\n", temp1, tambient);

            nanosleep(&sleep_time, NULL);
        }

        Log_Debug("Leaving main loop\n");
    }

    close_peripherals_and_handlers();

    Log_Debug("*** Terminated ***\n");
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

    if (result == 0)
    {
        epoll_fd = CreateEpollFd();
        if (epoll_fd < 0) 
        {
            result = -1;
        }
    }

    return result;
}

static int
init_peripherals(I2C_InterfaceId isu_id)
{
    int result = -1;

    // Initialize I2C bus
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

    // Initialize MLX90614 sensor
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

    // Initialize development kit button GPIO
    // Open button 1 GPIO as input
    if (result != -1)
    {
        Log_Debug("Opening PROJECT_BUTTON_1 as input.\n");
        button1_gpio_fd = GPIO_OpenAsInput(PROJECT_BUTTON_1);
        if (button1_gpio_fd < 0) {
            Log_Debug("ERROR: Could not open button GPIO: %s (%d).\n",
                strerror(errno), errno);
            result = -1;
        }
    }

    // Create timer for button press check
    if (result != -1)
    {
        struct timespec button_press_check_period = { 0, 1000000 };
        button_poll_timer_fd = CreateTimerFdAndAddToEpoll(epoll_fd,
            &button_press_check_period, &button_event_data, EPOLLIN);
        if (button_poll_timer_fd < 0)
        {
            Log_Debug("ERROR: Could not create button poll timer: %s (%d).\n",
                strerror(errno), errno);
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

static void
button1_press_handler(void)
{
    Log_Debug("Button1 pressed.\n");
    gb_is_termination_requested = true;
}

static void
button_timer_event_handler(EventData *event_data)
{
    GPIO_Value_Type new_btn1_state;
    bool b_is_all_ok = true;

    // Consume timer event
    if (ConsumeTimerFdEvent(button_poll_timer_fd) != 0) 
    {
        // Failed to consume timer event
        gb_is_termination_requested = true;
        b_is_all_ok = false;
    }

    // Check for a button1 press
    if (b_is_all_ok && (GPIO_GetValue(button1_gpio_fd, &new_btn1_state) != 0))
    {
        // Failed to get GPIO pin value
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n",
            strerror(errno), errno);
        gb_is_termination_requested = true;
        b_is_all_ok = false;
    }

    if (b_is_all_ok && (new_btn1_state != button1_state))
    {
        if (new_btn1_state == GPIO_Value_Low)
        {
            button1_press_handler();
        }
        button1_state = new_btn1_state;
    }

    return;
}

/* [] END OF FILE */

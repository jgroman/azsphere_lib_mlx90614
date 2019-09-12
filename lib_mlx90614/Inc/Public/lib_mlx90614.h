/***************************************************************************//**
* @file    lib_mlx90614.h
* @version 1.0.0
*
* @brief .
*
* @author   Jaroslav Groman
*
* @date
*
*******************************************************************************/

#ifndef _LIB_MLX90614_H_
#define _LIB_MLX90614_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <applibs/i2c.h>

// Uncomment line below to enable debugging messages
#define MLX90614_DEBUG

#define MLX90614_I2C_ADDRESS    0x5A

// RAM cells
// Note: If the RAM is read, the data are divided by two, due to a sign bit in
// RAM (for example, TOBJ1 - RAM address 0x07 will sweep between 0x27AD 
// to 0x7FFF as the object temperature changes from -70.01°C to + 382.19°C).
// The MSB read from RAM is an error flag (active high) for the linearized
// temperatures (TOBJ1, TOBJ2 and TA).
// The MSB for the raw data (e.g.IR sensor1 data) is a sign bit 
// (sign and magnitude format).
#define MLX90614_RREG_RAWIR1         0x04  // Raw data IR channel 1
#define MLX90614_RREG_RAWIR2         0x05  // Raw data IR channel 2
#define MLX90614_RREG_TA             0x06  // Linearized ambient temperature
#define MLX90614_RREG_TOBJ1          0x07  // Linearized object 1 temperature
#define MLX90614_RREG_TOBJ2          0x08  // Linearized object 2 temperature

// EEPROM cells
// Note: A write of 0x0000 must be done prior to writing in EEPROM in order 
// to erase the EEPROM cell content
#define MLX90614_EREG_TOMAX          0x20  // TOBJ range max
#define MLX90614_EREG_TOMIN          0x21  // TOBJ range min
#define MLX90614_EREG_PWMCTRL        0x22  // PWM Control
#define MLX90614_EREG_TA_RANGE       0x23  // TA Range, MSB:max, LSB:min
#define MLX90614_EREG_ECC            0x24  // Emissivity correction coefficient
#define MLX90614_EREG_CONF1          0x25  // Config Register 1
#define MLX90614_EREG_SMBUS_ADDR     0x2E  // SMBus address (LSByte only)
#define MLX90614_EREG_ID1            0x3C  // Device ID word 1
#define MLX90614_EREG_ID2            0x3D  // Device ID word 2
#define MLX90614_EREG_ID3            0x3E  // Device ID word 3
#define MLX90614_EREG_ID4            0x3F  // Device ID word 4

// Special commands
#define MLX90614_CMD_READ_FLAGS      0xF0
#define MLX90614_CMD_SLEEP_MODE      0xFF


// Value to indicate error when processing temperature measurement
#define MLX90614_TEMP_ERROR         -999.9

// Value to indicate error when processing emissivity
#define MLX90614_EMISSIVITY_ERROR   -1.0

// READ_FLAGS bitfields
typedef struct
{
    union
    {
        struct
        {
            uint8_t RSVD0 : 3;      // All zeros
            uint8_t RSVD3 : 1;      // Not implemented

            // POR initialization routine is still ongoing. Low active.
            uint8_t INIT  : 1;

            // EEPROM double error has occurred. High active.
            uint8_t EE_DEAD : 1;

            uint8_t RSVD6 : 1;      // Unused

            // The previous write/erase EEPROM access is still in progress. 
            // High active.
            uint8_t EEBUSY : 1;

            uint8_t RSVD8 : 8;      // All zeros

        };
        uint16_t word;
    };
} mlx90614_read_flags_t;

// PWMCTRL bitfields
typedef struct
{
    union
    {
        struct
        {
            // 0 - PWM extended mode. 1 - PWM single mode
            uint8_t PWM_MODE : 1;

            // 0 - PWM mode disabled. 1 - PWM mode enabled.
            uint8_t EN_PWM : 1;

            // 0 - SDA pin configured as Open Drain
            // 1 - SDA pin configured as Push-Pull
            uint8_t PPODB : 1;

            // 0 - PWM mode selected, 1 - Thermal relay mode selected
            uint8_t TRPWMB : 1;

            // PWM repetition number 0…62 step 2
            uint8_t PWM_REP : 5;

            // PWM Period
            uint8_t PWM_PERIOD : 7;
        };
        uint16_t word;
    };
} mlx90614_pwmctrl_t;

// CONF1 bitfields
typedef struct
{
    union
    {
        struct
        {
            // IIR filter parameter set
            uint8_t IIR : 3;

            // DO NOT MODIFY! This will cancel the factory calibration.
            uint8_t RPT_SENSOR_TEST : 1;

            uint8_t T_SEL : 2;

            // 0 - Single IR sensor, 1 - Dual IR sensor
            uint8_t SENSOR_MODE : 1;

            // DO NOT MODIFY
            uint8_t KS_SIGN : 1;

            // FIR filter parameter set
            uint8_t FIR : 3;

            // DO NOT MODIFY
            uint8_t GAIN : 3;

            // DO NOT MODIFY
            uint8_t KT2_SIGN : 1;

            // 0 - Enable sensor test, 1 - Disable sensor test
            uint8_t SENSOR_TEST : 1;
        };
        uint16_t word;
    };
} mlx90614_conf1_t;

#define CONF1_IIR_100     4   // IIR (100%) a1=1, b1=0
#define CONF1_IIR_80      5   // IIR (80%) a1=0.8, b1=0.2
#define CONF1_IIR_67      6   // IIR (67%) a1=0.666, b1=0.333
#define CONF1_IIR_57      7   // IIR (57%) a1=0.571, b1=0.428
#define CONF1_IIR_50      0   // IIR (50%) a1=0.5, b1=0.5
#define CONF1_IIR_25      1   // IIR (25%) a1=0.25, b1=0.75
#define CONF1_IIR_17      2   // IIR (17%) a1=0.166(6), b1=0.83(3)
#define CONF1_IIR_13      3   // IIR (13%) a1=0.125, b1=0.875

#define CONF1_T_SEL_A_1   0   // Ta, Tobj1
#define CONF1_T_SEL_A_2   1   // Ta, Tobj2
#define CONF1_T_SEL_2     2   // Tobj2
#define CONF1_T_SEL_1_2   3   // Tobj1, Tobj2

#define CONF1_FIR_8       0   // FIR = 8  not recommended
#define CONF1_FIR_16      1   // FIR = 16 not recommended
#define CONF1_FIR_32      2   // FIR = 32 not recommended
#define CONF1_FIR_64      3   // FIR = 64 not recommended
#define CONF1_FIR_128     4   // FIR = 128
#define CONF1_FIR_256     5   // FIR = 256
#define CONF1_FIR_512     6   // FIR = 512
#define CONF1_FIR_1024    7   // FIR = 1024

typedef enum {
    MLX_TEMP_LINEARIZED,
    MLX_TEMP_KELVIN,
    MLX_TEMP_CELSIUS,
    MLX_TEMP_FAHRENHEIT
} mlx_temperature_unit;

typedef struct mlx90614_struct
{
    int i2c_fd;                             // I2C interface file descriptor
    I2C_DeviceAddress i2c_addr;             // I2C device address
    uint16_t device_id[4];                  // 4x word Device ID
    mlx_temperature_unit temperature_unit;  // Temperature measurement unit
} mlx90614_t;

/**
 * @brief Initialize MLX90614 sensor.
 * @param i2c_fd I2C interface file descriptor.
 * @param i2c_addr Sensor I2C address.
 *
 * @return Pointer to MLX90614 device descriptor.
 */
mlx90614_t
*mlx90614_open(int i2c_fd, I2C_DeviceAddress i2c_addr);

/**
 * @brief Disables sensor and frees allocated resources.
 * @param p_mlx Pointer to MLX90614 device descriptor.
 */
void
mlx90614_close(mlx90614_t *p_mlx);

void
mlx90614_set_temperature_unit(mlx90614_t *p_mlx, mlx_temperature_unit unit);

bool
mlx90614_get_id(mlx90614_t *p_mlx);

I2C_DeviceAddress
mlx90614_get_address(mlx90614_t *p_mlx);

bool
mlx90614_set_address(mlx90614_t *p_mlx, I2C_DeviceAddress address);

float
mlx90614_get_temperature_object1(mlx90614_t *p_mlx);

float
mlx90614_get_temperature_object2(mlx90614_t *p_mlx);

float
mlx90614_get_temperature_ambient(mlx90614_t *p_mlx);

float
mlx90614_get_emissivity(mlx90614_t *p_mlx);

bool
mlx90614_set_emissivity(mlx90614_t *p_mlx, float emissivity);


// The following functions are useful only in PWM mode
// Range params are used for customizing the temperature range for PWM output

float
mlx90614_get_tobj_range_min(mlx90614_t *p_mlx);

bool
mlx90614_set_tobj_range_min(mlx90614_t *p_mlx, float t_min);

float
mlx90614_get_tobj_range_max(mlx90614_t *p_mlx);

bool
mlx90614_set_tobj_range_max(mlx90614_t *p_mlx, float t_max);

float
mlx90614_get_ta_range_min(mlx90614_t *p_mlx);

float
mlx90614_get_ta_range_max(mlx90614_t *p_mlx);


#ifdef __cplusplus
}
#endif

#endif  // _LIB_MLX90614_H_

/* [] END OF FILE */

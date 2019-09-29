/***************************************************************************//**
* @file    mlx90614_support.h
* @version 1.0.0
*
* @brief Support functions for MLX90614 IR sensor Azure Sphere library.
*
* @author   Jaroslav Groman
*
*******************************************************************************/

#ifndef _MLX90614_COMMON_H_
#define _MLX90614_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lib_mlx90614.h"

#ifdef MLX90614_DEBUG
#define MLX_DEBUG(s, f, ...) mlx90614_log_printf("%s %s: " s "\n", "MLX", f, \
                                                                 ## __VA_ARGS__)
#define MLX_DEBUG_DEV(s, f, d, ...) mlx90614_log_printf("%s %s (0x%02X): " s \
                                    "\n", "MLX", f, d->i2c_addr, ## __VA_ARGS__)
#else
#define MLX_DEBUG(s, f, ...)
#define MLX_DEBUG_DEV(s, f, d, ...)
#endif // MLX90614_DEBUG

#define MLX_ERROR(s, f, ...) mlx90614_log_printf("%s %s: " s "\n", "MLX90614", \
                                                              f, ## __VA_ARGS__)

// Uncomment line below to see I2C debug data
// #define MLX90614_I2C_DEBUG

#define MLX90614_T_ERASE_MS     5   // Erase EEPROM cell delay
#define MLX90614_T_WRITE_MS     5   // Write EEPROM cell delay

/**
 * @brief Platform dependent log print function.
 *
 * @param p_format The message string to log.
 * @param ... Argument list.
 *
 * @result 0 for success, or -1 for failure, in which case errno is set 
 * to the error value.
 */
int
mlx90614_log_printf(const char *p_format, ...);

/**
 * @brief Read MLX90614 register contents.
 *
 * @param p_mlx Pointer to MLX90614 device descriptor.
 * @param reg_addr Reagister address.
 * @param p_reg_value Pointer to variable to store register contents.
 *
 * @result True for success, or false for failure.
 */
bool
mlx90614_reg_read(mlx90614_t *p_mlx, uint8_t reg_addr, int16_t *p_reg_value);

/**
 * @brief Write value to MLX90614 RAM register.
 *
 * @param p_mlx Pointer to MLX90614 device descriptor.
 * @param reg_addr Reagister address.
 * @param reg_value Value to write.
 *
 * @result True for success, or false for failure.
 */
bool
mlx90614_reg_write(mlx90614_t *p_mlx, uint8_t reg_addr, int16_t reg_value);

/**
 * @brief Write value to MLX90614 EEPROM register.
 *
 * @param p_mlx Pointer to MLX90614 device descriptor.
 * @param reg_addr Reagister address.
 * @param reg_value Value to write.
 *
 * @result True for success, or false for failure.
 */
bool
mlx90614_eeprom_write(mlx90614_t *p_mlx, uint8_t reg_addr, int16_t reg_value);

#ifdef __cplusplus
}
#endif

#endif  // _MLX90614_COMMON_H_

/* [] END OF FILE */

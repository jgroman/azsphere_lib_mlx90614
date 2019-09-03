

#ifndef _MLX90614_COMMON_H_
#define _MLX90614_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lib_mlx90614.h"

#ifdef MLX90614_DEBUG
#define DEBUG(s, f, ...) log_printf("%s %s: " s "\n", "MLX", f, ## __VA_ARGS__)
#define DEBUG_DEV(s, f, d, ...) log_printf("%s %s (0x%02X): " s "\n", "MLX", f, d->i2c_addr, ## __VA_ARGS__)
#else
#define DEBUG(s, f, ...)
#define DEBUG_DEV(s, f, d, ...)
#endif // MLX90614_DEBUG

#define ERROR(s, f, ...) log_printf("%s %s: " s "\n", "MLX90614", f, ## __VA_ARGS__)

// Uncomment line below to see I2C debug data
#define MLX90614_I2C_DEBUG

#define MLX90614_T_ERASE_MS     5   // Erase EEPROM cell time
#define MLX90614_T_WRITE_MS     5   // Write EEPROM cell time

int
log_printf(const char *p_format, ...);

bool
reg_read(mlx90614_t *p_mlx, uint8_t reg_addr, uint16_t *p_reg_value);

bool
reg_write(mlx90614_t *p_mlx, uint8_t reg_addr, uint16_t reg_value);

bool
eeprom_write(mlx90614_t *p_mlx, uint8_t reg_addr, uint16_t reg_value);

#ifdef __cplusplus
}
#endif

#endif  // _MLX90614_COMMON_H_

/* [] END OF FILE */
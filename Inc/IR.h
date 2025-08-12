#ifndef IR_H_
#define IR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes GPIO lines for I2C communication (PB10 = SCL, PB11 = SDA)
 */
void IR_Init(void);

/**
 * @brief Reads raw 16-bit temperature register from the sensor
 * @return 16-bit raw value from AIT1001 register 0x01
 */
uint16_t IR_ReadRaw(void);

/**
 * @brief Reads temperature and converts to Celsius
 * @return Temperature in degrees Celsius as float
 */
float IR_ReadTemperature(void);

#ifdef __cplusplus
}
#endif

#endif /* IR_H_ */

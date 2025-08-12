/*
 * m95640_driver.h
 *
 *  Created on: Jul 22, 2025
 *      Author: ADE1HYD
 */

#ifndef EEPROM_HAL_H
#define EEPROM_HAL_H

#include "main.h" // Standard HAL includes

// ---- Public Variables ----
extern SPI_HandleTypeDef hspi1; // Make the SPI handle available externally

// ---- Public Function Prototypes ----

/**
  * @brief  Initializes the SPI peripheral and GPIO pins for the EEPROM.
  * @retval HAL_StatusTypeDef: HAL_OK if successful.
  */
HAL_StatusTypeDef SpiEepromInit_HAL(void);

/**
  * @brief  Writes a single byte to the EEPROM at a specific address.
  * @param  add: The 16-bit address to write to.
  * @param  data: The byte of data to write.
  */
void EepromWriteByte_HAL(uint16_t add, uint8_t data);

/**
  * @brief  Reads a single byte from the EEPROM at a specific address.
  * @param  add: The 16-bit address to read from.
  * @retval uint8_t: The data byte read from the EEPROM.
  */
uint8_t EepromReadByte_HAL(uint16_t add);

/**
  * @brief  Writes a buffer of data to the EEPROM.
  * @param  add: The starting address to write to.
  * @param  pData: Pointer to the data buffer to write.
  * @param  len: The number of bytes to write.
  */
void EepromWriteBuffer_HAL(uint16_t add, uint8_t* pData, uint16_t len);

/**
  * @brief  Reads a buffer of data from the EEPROM.
  * @param  add: The starting address to read from.
  * @param  pData: Pointer to the buffer to store the read data.
  * @param  len: The number of bytes to read.
  */
void EepromReadBuffer_HAL(uint16_t add, uint8_t* pData, uint16_t len);


#endif // EEPROM_HAL_H

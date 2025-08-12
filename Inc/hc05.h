/*
 * hc05.h
 *
 *  Created on: Jul 24, 2025
 *      Author: ADE1HYD
 */

#ifndef INC_HC05_H_
#define INC_HC05_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

// --- Configuration ---
// Define the UART handle the HC-05 is connected to
// This must be configured in your STM32CubeMX project or main.c
// Example: extern UART_HandleTypeDef huart2;
#define HC05_UART_HANDLE      huart2

// Define the size of the receive buffer
#define HC05_RX_BUFFER_SIZE   256

// --- Public Function Prototypes ---

/**
 * @brief Initializes the HC-05 driver.
 * @param huart: Pointer to the UART_HandleTypeDef structure that will be used.
 * @note This function enables the UART receive interrupt. Ensure the corresponding
 * UART global interrupt is enabled in the NVIC.
 */
void hc05_init(UART_HandleTypeDef *huart);

/**
 * @brief Checks if there is data available in the receive buffer.
 * @return uint16_t: The number of bytes available to read.
 */
uint16_t hc05_available(void);

/**
 * @brief Reads one byte from the receive buffer.
 * @return int16_t: The received byte (0-255) or -1 if the buffer is empty.
 */
int16_t hc05_read(void);

/**
 * @brief Sends a single byte over UART.
 * @param byte: The byte to send.
 */
void hc05_send_byte(uint8_t byte);

/**
 * @brief Sends an array of data over UART.
 * @param data: Pointer to the data array.
 * @param size: Number of bytes to send.
 */
void hc05_send_data(uint8_t *data, uint16_t size);

/**
 * @brief Sends a null-terminated string over UART.
 * @param str: Pointer to the string.
 */
void hc05_send_string(const char *str);

/**
 * @brief The callback function that must be called from the main UART RxCpltCallback.
 * @param huart: Pointer to the UART handle that triggered the interrupt.
 */
void hc05_uart_rx_callback(UART_HandleTypeDef *huart);


#endif /* INC_HC05_H_ */

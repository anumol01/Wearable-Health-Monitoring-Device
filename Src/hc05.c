/*
 * hc05.c
 *
 *  Created on: Jul 24, 2025
 *      Author: ADE1HYD
 */


#include "hc05.h"
#include <string.h>

// --- Private Variables ---

// Pointer to the UART handle
static UART_HandleTypeDef *hc05_huart;

// Circular buffer for receiving data
static uint8_t rx_buffer[HC05_RX_BUFFER_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

// Single byte buffer for HAL's interrupt reception
static uint8_t rx_byte;

// --- Function Implementations ---

/**
 * @brief Initializes the HC-05 driver.
 */
void hc05_init(UART_HandleTypeDef *huart) {
    hc05_huart = huart;
    // Start listening for incoming data.
    // HAL_UART_Receive_IT will place one received byte into rx_byte and
    // then call the HAL_UART_RxCpltCallback.
//    HAL_UART_Receive_IT(hc05_huart, &rx_byte, 1);
}

/**
 * @brief Checks if there is data available in the receive buffer.
 */
uint16_t hc05_available(void) {
    // Calculate the number of bytes in the buffer
    if (rx_head >= rx_tail) {
        return rx_head - rx_tail;
    } else {
        return HC05_RX_BUFFER_SIZE - rx_tail + rx_head;
    }
}

/**
 * @brief Reads one byte from the receive buffer.
 */
int16_t hc05_read(void) {
    // Check if buffer is empty
    if (rx_tail == rx_head) {
        return -1; // No data available
    }

    // Get byte from buffer
    uint8_t byte = rx_buffer[rx_tail];
    // Move tail pointer
    rx_tail = (rx_tail + 1) % HC05_RX_BUFFER_SIZE;
    return byte;
}

/**
 * @brief Sends a single byte over UART.
 */
void hc05_send_byte(uint8_t byte) {
    HAL_UART_Transmit(hc05_huart, &byte, 1, HAL_MAX_DELAY);
}

/**
 * @brief Sends an array of data over UART.
 */
void hc05_send_data(uint8_t *data, uint16_t size) {
    HAL_UART_Transmit(hc05_huart, data, size, HAL_MAX_DELAY);
}

/**
 * @brief Sends a null-terminated string over UART.
 */
void hc05_send_string(const char *str) {
    uint16_t len = strlen(str);
    HAL_UART_Transmit(hc05_huart, (uint8_t*)str, len, HAL_MAX_DELAY);
}

/**
 * @brief The UART receive complete callback to be called from the main ISR.
 * @note This function implements the circular buffer logic.
 */
void hc05_uart_rx_callback(UART_HandleTypeDef *huart) {
    // Check if the interrupt is from the correct UART instance
    if (huart->Instance == hc05_huart->Instance) {
        // Calculate next head position
        uint16_t next_head = (rx_head + 1) % HC05_RX_BUFFER_SIZE;

        // Check for buffer overflow
        if (next_head != rx_tail) {
            // No overflow, add the byte to the buffer
            rx_buffer[rx_head] = rx_byte;
            rx_head = next_head;
        } else {
            // Buffer overflow occurred. The new byte is discarded.
            // You could add error handling here if needed.
        }

        // Re-arm the UART receive interrupt to listen for the next byte
        HAL_UART_Receive_IT(hc05_huart, &rx_byte, 1);
    }
}

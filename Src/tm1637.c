/*
 * tm1637.c
 *
 *  Created on: May 19, 2025
 *      Author: ADE1HYD
 */


#include "tm1637.h"
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

static const uint8_t segmentMap[] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66,
    0x6d, 0x7d, 0x07, 0x7f, 0x6f
};

static void TM1637_Delay_us(uint16_t us) {
    for (volatile uint32_t i = 0; i < us * 8; i++);
}

static void TM1637_Start(TM1637_Display* display) {
    HAL_GPIO_WritePin(display->dio_port, display->dio_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(display->clk_port, display->clk_pin, GPIO_PIN_SET);
    TM1637_Delay_us(5);
    HAL_GPIO_WritePin(display->dio_port, display->dio_pin, GPIO_PIN_RESET);
}

static void TM1637_Stop(TM1637_Display* display) {
    HAL_GPIO_WritePin(display->clk_port, display->clk_pin, GPIO_PIN_RESET);
    TM1637_Delay_us(5);
    HAL_GPIO_WritePin(display->dio_port, display->dio_pin, GPIO_PIN_RESET);
    TM1637_Delay_us(5);
    HAL_GPIO_WritePin(display->clk_port, display->clk_pin, GPIO_PIN_SET);
    TM1637_Delay_us(5);
    HAL_GPIO_WritePin(display->dio_port, display->dio_pin, GPIO_PIN_SET);
}

static void TM1637_WriteByte(TM1637_Display* display, uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(display->clk_port, display->clk_pin, GPIO_PIN_RESET);
        if (byte & 0x01)
            HAL_GPIO_WritePin(display->dio_port, display->dio_pin, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(display->dio_port, display->dio_pin, GPIO_PIN_RESET);
        TM1637_Delay_us(3);
        HAL_GPIO_WritePin(display->clk_port, display->clk_pin, GPIO_PIN_SET);
        TM1637_Delay_us(3);
        byte >>= 1;
    }

    // Wait for ACK
    HAL_GPIO_WritePin(display->clk_port, display->clk_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(display->dio_port, display->dio_pin, GPIO_PIN_SET);
    TM1637_Delay_us(3);
    HAL_GPIO_WritePin(display->clk_port, display->clk_pin, GPIO_PIN_SET);
    TM1637_Delay_us(3);
    HAL_GPIO_WritePin(display->clk_port, display->clk_pin, GPIO_PIN_RESET);
}

void TM1637_Init(TM1637_Display* display) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable GPIO clock (assuming all displays use GPIOB; adjust if needed)
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = display->clk_pin | display->dio_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(display->clk_port, &GPIO_InitStruct);

    // Initialize brightness
    display->brightness = 0x07; // Default brightness
}

void TM1637_DisplayDecimal(TM1637_Display* display, int num) {
    uint8_t digits[4];

    for (int i = 0; i < 4; i++) {
        digits[3 - i] = segmentMap[num % 10];
        num /= 10;
    }

    TM1637_Start(display);
    TM1637_WriteByte(display, 0x40); // Command for data write
    TM1637_Stop(display);

    TM1637_Start(display);
    TM1637_WriteByte(display, 0xC0); // Address command
    for (int i = 0; i < 4; i++) {
        TM1637_WriteByte(display, digits[i]);
    }
    TM1637_Stop(display);

    TM1637_Start(display);
    TM1637_WriteByte(display, 0x88 | (display->brightness & 0x07)); // Display control
    TM1637_Stop(display);
}

void TM1637_CountdownSeconds(TM1637_Display* display, uint16_t seconds) {
    char buffer[5];

    for (int i = seconds; i >= 1; i--) {
        snprintf(buffer, 5, "%d", i);

        // Pad with spaces to make it 4 characters wide (right-aligned)
        int len = strlen(buffer);
        char displayStr[5] = "    ";
        memcpy(&displayStr[4 - len], buffer, len);

        TM1637_DisplayString(display, displayStr);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
//void TM1637_CountdownSeconds(TM1637_Display* display, uint16_t seconds) {
//    char buffer[5];
//    for (uint16_t i = seconds; i >= 0; i--) {
//        snprintf(buffer, 5, "%04d", i); // Pad with leading zeros
//        TM1637_DisplayString(display, buffer);
//        if (i > 0) {
//            vTaskDelay(pdMS_TO_TICKS(1000)); // Delay only if not at 0
//        }
//    }
//}

void TM1637_DisplayString(TM1637_Display* display, const char* str) {
    uint8_t segments[4] = {0};

    for (int i = 0; i < 4; i++) {
        if (str[i] == ' ') {
            segments[i] = 0x00; // Blank
        } else if (str[i] >= '0' && str[i] <= '9') {
            segments[i] = segmentMap[str[i] - '0'];
        } else {
            segments[i] = 0x00; // Unknown characters -> blank
        }
    }

    TM1637_Start(display);
    TM1637_WriteByte(display, 0x40); // Automatic address increment
    TM1637_Stop(display);

    TM1637_Start(display);
    TM1637_WriteByte(display, 0xC0); // Start address
    for (int i = 0; i < 4; i++) {
        TM1637_WriteByte(display, segments[i]);
    }
    TM1637_Stop(display);

    TM1637_Start(display);
    TM1637_WriteByte(display, 0x88 | (display->brightness & 0x07)); // Set brightness
    TM1637_Stop(display);
}

void TM1637_SetBrightness(TM1637_Display* display, uint8_t b) {
    display->brightness = b & 0x07; // Limit to 0–7
}

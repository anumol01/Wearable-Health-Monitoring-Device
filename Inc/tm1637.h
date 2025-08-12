/*
 * tm1637.h
 *
 *  Created on: May 19, 2025
 *      Author: ADE1HYD
 */

#ifndef TM1637_H
#define TM1637_H

#include "stm32f4xx_hal.h"

// Structure to hold TM1637 display configuration
typedef struct {
    GPIO_TypeDef* clk_port;
    uint16_t clk_pin;
    GPIO_TypeDef* dio_port;
    uint16_t dio_pin;
    uint8_t brightness;
} TM1637_Display;

// Function prototypes
void TM1637_Init(TM1637_Display* display);
void TM1637_DisplayDecimal(TM1637_Display* display, int num);
void TM1637_SetBrightness(TM1637_Display* display, uint8_t brightness);
void TM1637_DisplayString(TM1637_Display* display, const char* str);
void TM1637_CountdownSeconds(TM1637_Display* display, uint16_t seconds);

#endif



# Wearable Health Monitoring Device

## Overview

This project implements a wearable health monitoring system using an STM32F405 microcontroller. It measures vital signs such as heart rate, SpO₂, and body temperature, provides user access control, displays results, and indicates overall health status.

## Features

* **Vital Signs Monitoring**: Heart rate, SpO₂, and temperature measurement.
* **Security Access**: 4-digit password protection stored in SPI EEPROM.
* **Display**:

  * 4-digit 7-segment LEDs for heart rate and SpO₂.
  * LCD for temperature and menu prompts.
* **Health Indicators**:

  * RGB LEDs and LED bar graph for status visualization.
  * Buzzer alerts for abnormal readings.
* **Data Communication**: Sends readings to PC via UART.

## Hardware Components

* STM32F405 board
* Non-contact IR temperature sensor
* 4-digit 7-segment LED (x2)
* LCD display
* RGB LEDs & LED bar graph
* 4x4 matrix keypad
* Buzzer
* Buck converter

## Software Requirements

* STM32CubeIDE
* FreeRTOS for task scheduling
* HAL drivers for peripheral handling

## Operation Flow

1. Power on → LCD shows "Welcome".
2. Press `*` → Enter 4-digit code and press `#`.
3. If correct → "Access Granted" & Menu options:

   * Change password
   * Enter system
4. System measures and displays heart rate, SpO₂, and temperature.
5. Alerts & LED indicators show health status.
6. Readings sent to PC via UART.

## Deliverables

* Complete code
* Circuit diagrams & photos
* Output screenshots & live expressions
* Project documentation



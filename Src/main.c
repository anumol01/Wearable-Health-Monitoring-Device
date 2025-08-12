/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "m95640_driver.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "lcd.h"
#include "semphr.h"
#include "stm32f405xx.h"
#include "IR.h"
#include "hc05.h"
#include <stdint.h>
#include "tm1637.h"
SemaphoreHandle_t passkey_semaphore;
#define EEPROM_ADDRESS 0X0000  //
// === Keypad Configuration ===
const char keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
char key = 0;

uint32_t taskprofiler = 0;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
void GPIO_Init(void);
void Task_Keypad(void *params);
char Keypad_Scan(void);
void displayTask(void *param);
void led_control(void);
void TempReadTask(void *params);

TM1637_Display spo2_display = {
		.clk_port = spo2_clk_GPIO_Port,
		.clk_pin = spo2_clk_Pin,
		.dio_port = spo2_do_GPIO_Port,
		.dio_pin = spo2_do_Pin,
		.brightness = 7
};
TM1637_Display heart_display = {
		.clk_port = heart_clk_GPIO_Port,
		.clk_pin = heart_clk_Pin,
		.dio_port = heart_do_GPIO_Port,
		.dio_pin = heart_do_Pin,
		.brightness = 7
};



// === GPIO Init ===
void GPIO_Init(void)
{
    RCC->AHB1ENR |= (1 << 0) | (1 << 1) | (1 << 2); // Enable GPIOA, GPIOB, GPIOC

    // PB2–PB5: Rows as output
    for (int i = 0; i < 4; i++) {
        GPIOB->MODER &= ~(3 << (i * 2));
        GPIOB->MODER |= (1 << (i * 2));
        GPIOB->ODR |= (1 << i); // Set high
    }

    // PB6–PB7: Columns as input pull-up
    for (int i = 4; i < 8; i++) {
        GPIOB->MODER &= ~(3 << (i * 2));
        GPIOB->PUPDR &= ~(3 << (i * 2));
        GPIOB->PUPDR |= (1 << (i * 2));
    }
    for (int i = 0; i <= 5; i++)
    GPIOC->MODER |= (1 << (i * 2));  // PCx as output

    // PC9 (Green) and PC10 (Red): LED output
    GPIOC->MODER &= ~(3 << (8 * 2));
    GPIOC->MODER |= (1 << (8 * 2));
    GPIOC->MODER &= ~(3 << (12 * 2));
    GPIOC->MODER |= (1 << (12 * 2));
}


// === LED Bar Graph Based on ADC Input ===
void led_control(void)
{
    //GPIOC->ODR &= ~((1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 5)); // Clear PC0,1,3,4,5
    GPIOC->ODR |= (1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 5);
//    if (adc_value < 300) {
//        GPIOC->ODR |= (1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 5);
//    }
//    else if (adc_value < 500) {
//        GPIOC->ODR |= (1 << 0) | (1 << 1) | (1 << 3) | (1 << 4);
//    }
//    else if (adc_value < 1000) {
//        GPIOC->ODR |= (1 << 0) | (1 << 1) | (1 << 3);
//    }
//    else if (adc_value < 1500) {
//        GPIOC->ODR |= (1 << 0) | (1 << 1);
//    }
//    else if (adc_value < 2000) {
//        GPIOC->ODR |= (1 << 0);
//    }
//    else {
//        // No LEDs ON for adc >= 2000 (brightest)
//    }
}

// === Keypad Scan ===
char Keypad_Scan(void)
{
    const uint8_t row_pins[4] = {0, 1, 2, 3};        // PB2–PB5
    const uint8_t col_pins_B[4] = {4, 5, 6, 7};      // PB6–PB7


    for (int row = 0; row < 4; row++)
    {
        // Set all rows high first
        for (int r = 0; r < 4; r++)
            GPIOB->ODR |= (1 << row_pins[r]);

        // Pull current row low
        GPIOB->ODR &= ~(1 << row_pins[row]);

        vTaskDelay(pdMS_TO_TICKS(2)); // Debounce

        // Check PB6, PB7 (Columns)
        for (int col = 0; col < 4; col++)
        {
        	if ((GPIOB->IDR & (1 << col_pins_B[col])) == 0)
        	{
        	    vTaskDelay(pdMS_TO_TICKS(20));  // Debounce delay

        	    if ((GPIOB->IDR & (1 << col_pins_B[col])) == 0)  // Confirm key still pressed
        	    {
        	        while ((GPIOB->IDR & (1 << col_pins_B[col])) == 0); // Wait for release
        	        return keymap[row][col];
        	    }
        	}

        }
    }

    return 0; // No key press detected
}


uint8_t rx_buffer[4];

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// === Keypad Scan ===

void Task_Keypad(void *params)

{
    char entered[5] = {0};      // 4 digits + null
    char stored_pass[5] = {0};
    char key = 0;
    int key_index = 0;
    lprint(0x80, "Welcome");
    vTaskDelay(pdMS_TO_TICKS(1000));
    char string_to_write[5] = "1234";
    EepromWriteBuffer_HAL(EEPROM_ADDRESS, (uint8_t*)string_to_write, strlen(string_to_write) + 1);
    // Read 4-digit password from EEPROM (address 0x0000)
    EepromReadBuffer_HAL(0x0000, (uint8_t *)stored_pass, 4);
    stored_pass[4] = '\0';
    while (1)
    {
        key_index = 0;
        memset(entered, 0, sizeof(entered));
        lprint(0x80, "Press * to Start   ");
        while ((key = Keypad_Scan()) != '*') {
        vTaskDelay(pdMS_TO_TICKS(50));
        }
        lprint(0x80, "Security Code:   ");
        lprint(0xC0, "                 ");
        while (1)
        {
            key = Keypad_Scan();
            if (key != 0)
            {
                if (key == '#' && key_index == 4)
                {
                    entered[4] = '\0';
                    if (strncmp(entered, stored_pass, 4) == 0)
                    {
                        lprint(0x80, "Access Granted   ");
                        GPIOC->ODR |= (1 << 8);   // GREEN ON
                        GPIOC->ODR &= ~(1 << 12); // RED OFF
                        vTaskDelay(pdMS_TO_TICKS(2000));
                        GPIOC->ODR &= ~(1 << 8); // GREEN OFF
                        // Show Menu
                        lprint(0x80, "Menu: A=Chg Pwd  ");
                        lprint(0xC0, "B=Enter System   ");
                        while (1)
                        {
                            key = Keypad_Scan();
                            if (key == 'A') {
                            lprint(0x80, "New 4-digit code:  ");
                            lprint(0xC0, "                 ");
							key_index = 0;
							memset(entered, 0, sizeof(entered));
                                while (1) {
                                    key = Keypad_Scan();
                                    if (key && key != '#' && key_index < 4) {
                                        entered[key_index++] = key;
                                        char buf[17] = "Entered:         ";
                                        strncpy(&buf[9], entered, key_index);
                                        buf[9 + key_index] = '\0';
                                        lprint(0xC0, buf);
                                    } else if (key == '#' && key_index == 4) {
                                        // Save to EEPROM
                                        EepromWriteBuffer_HAL(0x0000, (uint8_t *)entered, 4);
                                        strncpy(stored_pass, entered, 5);
                                        lprint(0x80, "Password Changed! ");
                                        lprint(0xC0, "                 ");
                                        vTaskDelay(pdMS_TO_TICKS(2000));
                                        break;
                                    }
                                    vTaskDelay(pdMS_TO_TICKS(50));
                                }
                                break; // Exit menu
                            } else if (key == 'B') {
                            	GPIOC->ODR |= (1 << 8);   // GREEN ON
                                lprint(0x80, "Entering System...    ");
                                lprint(0xC0, "                 ");
//                                xSemaphoreGive(passkey_semaphore);
                                xTaskCreate(TempReadTask, "TempTask", 256, NULL, 1, NULL);
                                vTaskDelay(pdMS_TO_TICKS(2000));
                                // Add system logic here
                                vTaskDelete(NULL);
                            }
                            vTaskDelay(pdMS_TO_TICKS(50));
                        }
                        break; // Go back to login screen
                    }
                    else
                    {
                        lprint(0x80, "Access Denied!     ");
                        lprint(0xC0, "                 ");
                        GPIOC->ODR |= (1 << 12);  // RED ON
                        GPIOC->ODR &= ~(1 << 8);  // GREEN OFF
                        vTaskDelay(pdMS_TO_TICKS(2000));
                        GPIOC->ODR &= ~(1 << 10); // RED OFF
                        break;
                    }
                }
                else if (key_index < 4 && key >= '0' && key <= '9')
                {
                    entered[key_index++] = key;
                    char buf[17] = "Entered:         ";
                    strncpy(&buf[9], entered, key_index);
                    buf[9 + key_index] = '\0';
                    lprint(0xC0, buf);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
void TempReadTask(void *params) {
//	xSemaphoreTake(passkey_semaphore, portMAX_DELAY);
    char buffer[32];
    int temp;

    IR_Init();
    LcdInit();

    while (1) {
        temp = (int)IR_ReadTemperature();
        sprintf(buffer, "Temp: %d C      \r\n", temp);

        lprint(0x80, buffer);

        HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);

        taskprofiler++;

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
//  passkey_semaphore = xSemaphoreCreateBinary();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  GPIO_Init();
  LcdInit();
  SpiEepromInit_HAL();
  TM1637_Init(&spo2_display);
  TM1637_Init(&heart_display);


  // Create Task


  xTaskCreate(Task_Keypad, "Keypad", 256, NULL, 2, NULL);
//  xTaskCreate(displayTask, "display", 128, NULL, 1, NULL);

    // Start Scheduler

    vTaskStartScheduler();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM3 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM3) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

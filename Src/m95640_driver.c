/*
 * m95640_driver.c
 *
 *  Created on: Jul 22, 2025
 *      Author: ADE1HYD
 */

#include "m95640_driver.h"
#include "spi.h"
// ---- Global SPI Handle Definition ----
extern SPI_HandleTypeDef hspi1;

// ---- Private Defines ----
// EEPROM Commands
#define CmdWriteEnable      0x06
#define CmdRead             0x03
#define CmdWrite            0x02

// Pin Definitions
#define PORT_CS             GPIOA
#define PIN_CS              GPIO_PIN_4

#define PORT_SCK            GPIOA
#define PIN_SCK             GPIO_PIN_5

#define PORT_MISO           GPIOA
#define PIN_MISO            GPIO_PIN_6

#define PORT_MOSI           GPIOA
#define PIN_MOSI            GPIO_PIN_7

// A standard timeout for blocking HAL calls
#define EEPROM_SPI_TIMEOUT  100

#define EEPROM_PAGE_SIZE    32

// ---- Private Function Prototypes ----
static void EepromWriteEnable_HAL(void);


// ---- Public Function Implementations ----

HAL_StatusTypeDef SpiEepromInit_HAL(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    HAL_StatusTypeDef status;

    // 1. Enable peripheral clocks
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // 2. Configure Chip Select (CS) pin as GPIO Output
    GPIO_InitStruct.Pin = PIN_CS;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PORT_CS, &GPIO_InitStruct);

    HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_SET); // Set CS high (inactive)

    // 3. Configure SCK, MISO, MOSI pins for SPI Alternate Function
    GPIO_InitStruct.Pin = PIN_SCK | PIN_MISO | PIN_MOSI;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 4. Configure the SPI peripheral
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

    status = HAL_SPI_Init(&hspi1);

    return status;
}

void EepromWriteByte_HAL(uint16_t add, uint8_t data)
{
    uint8_t tx_buffer[4];

    EepromWriteEnable_HAL(); // Enable writing

    // Prepare the command sequence
    tx_buffer[0] = CmdWrite;
    tx_buffer[1] = (add >> 8) & 0xFF; // Address High Byte
    tx_buffer[2] = add & 0xFF;        // Address Low Byte
    tx_buffer[3] = data;              // Data byte

    HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_RESET); // Select chip
    HAL_SPI_Transmit(&hspi1, tx_buffer, 4, EEPROM_SPI_TIMEOUT);
    HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_SET); // Deselect chip

    HAL_Delay(5); // Wait for the internal write cycle to complete
}

uint8_t EepromReadByte_HAL(uint16_t add)
{
    uint8_t tx_buffer[3];
    uint8_t rx_data;
    uint8_t dummy_byte = 0xFF;

    // Prepare the command sequence
    tx_buffer[0] = CmdRead;
    tx_buffer[1] = (add >> 8) & 0xFF; // Address High Byte
    tx_buffer[2] = add & 0xFF;        // Address Low Byte

    HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_RESET); // Select chip

    HAL_SPI_Transmit(&hspi1, tx_buffer, 3, EEPROM_SPI_TIMEOUT);
    HAL_SPI_TransmitReceive(&hspi1, &dummy_byte, &rx_data, 1, EEPROM_SPI_TIMEOUT);

    HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_SET); // Deselect chip

    return rx_data;
}


// ---- Private Function Implementations ----

/**
  * @brief  Sends the "Write Enable" command to the EEPROM.
  * @note   This is a private helper function.
  */
static void EepromWriteEnable_HAL(void)
{
    uint8_t command = CmdWriteEnable;

    HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &command, 1, EEPROM_SPI_TIMEOUT);
    HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_SET);
}


/**
  * @brief  Writes a buffer of data to the EEPROM.
  * @note   This function handles page boundary conditions automatically.
  */
void EepromWriteBuffer_HAL(uint16_t add, uint8_t* pData, uint16_t len)
{
    uint8_t header[3];
    uint16_t bytes_to_write;

    header[0] = CmdWrite;

    while (len > 0)
    {
        // Calculate how many bytes can be written before crossing a page boundary.
        // The page boundary is a multiple of EEPROM_PAGE_SIZE (32).
        uint16_t bytes_left_on_page = EEPROM_PAGE_SIZE - (add % EEPROM_PAGE_SIZE);

        // Determine the actual number of bytes for this transaction
        if (len > bytes_left_on_page)
        {
            bytes_to_write = bytes_left_on_page;
        }
        else
        {
            bytes_to_write = len;
        }

        // Prepare the write command and address
        header[1] = (add >> 8) & 0xFF;
        header[2] = add & 0xFF;

        // --- Perform the write transaction ---
        EepromWriteEnable_HAL(); // Enable writing

        HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_RESET); // Select chip

        // Send header (Cmd + Address) and then the data payload
        HAL_SPI_Transmit(&hspi1, header, 3, EEPROM_SPI_TIMEOUT);
        HAL_SPI_Transmit(&hspi1, pData, bytes_to_write, EEPROM_SPI_TIMEOUT);

        HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_SET); // Deselect chip

        HAL_Delay(5); // Wait for the internal write cycle to complete

        // Update counters for the next loop iteration
        len -= bytes_to_write;
        add += bytes_to_write;
        pData += bytes_to_write;
    }
}


/**
  * @brief  Reads a buffer of data from the EEPROM.
  * @note   Reading can cross page boundaries without issue.
  */
void EepromReadBuffer_HAL(uint16_t add, uint8_t* pData, uint16_t len)
{
    uint8_t header[3];

    // Prepare the read command and address
    header[0] = CmdRead;
    header[1] = (add >> 8) & 0xFF;
    header[2] = add & 0xFF;

    HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_RESET); // Select chip

    // Send the read command and address
    HAL_SPI_Transmit(&hspi1, header, 3, EEPROM_SPI_TIMEOUT);

    // Receive the data payload
    HAL_SPI_Receive(&hspi1, pData, len, EEPROM_SPI_TIMEOUT);

    HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_SET); // Deselect chip
}

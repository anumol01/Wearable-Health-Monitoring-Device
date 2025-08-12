// IR.c
#include "IR.h"
#include "stm32f405xx.h"
#include "core_cm4.h"  // Required for SysTick and __NOP()

#define IR_SDA 11  // PB11
#define IR_SCL 10  // PB10
#define SENSOR_ID 0x0A
#define IR_DELAY()  for (volatile int d = 0; d < 30; d++) __NOP()

// Private I2C utility functions
static void dataLineLow(void) {
    GPIOB->MODER &= ~(3 << (2 * IR_SDA));        // Clear mode
    GPIOB->MODER |=  (1 << (2 * IR_SDA));        // Set as output
    GPIOB->ODR &= ~(1 << IR_SDA);                // Drive low
}

static void dataLineHigh(void) {
    GPIOB->MODER &= ~(3 << (2 * IR_SDA));        // Set as input (Hi-Z)
}

static void clockLineLow(void) {
    GPIOB->MODER &= ~(3 << (2 * IR_SCL));
    GPIOB->MODER |=  (1 << (2 * IR_SCL));
    GPIOB->ODR &= ~(1 << IR_SCL);
}

static void clockLineHigh(void) {
    GPIOB->MODER &= ~(3 << (2 * IR_SCL));        // Input mode for pull-up
}

static uint8_t sampleSDA(void) {
    return (GPIOB->IDR & (1 << IR_SDA)) ? 1 : 0;
}

// I2C Start condition
static void startCondition(void) {
    dataLineHigh(); clockLineHigh(); IR_DELAY();
    dataLineLow(); IR_DELAY();
    clockLineLow(); IR_DELAY();
}

// I2C Stop condition
static void stopCondition(void) {
    dataLineLow(); IR_DELAY();
    clockLineHigh(); IR_DELAY();
    dataLineHigh(); IR_DELAY();
}

// Send one byte over I2C
static uint8_t sendByte(uint8_t value) {
    for (int j = 0; j < 8; j++) {
        (value & 0x80) ? dataLineHigh() : dataLineLow();
        IR_DELAY();
        clockLineHigh(); IR_DELAY();
        clockLineLow(); IR_DELAY();
        value <<= 1;
    }
    dataLineHigh(); IR_DELAY();  // Release SDA for ACK
    clockLineHigh(); IR_DELAY();
    uint8_t response = sampleSDA();  // Read ACK/NACK
    clockLineLow(); IR_DELAY();
    return response;
}

// Read one byte from I2C
static uint8_t readByte(uint8_t ack) {
    uint8_t byte = 0;
    dataLineHigh();  // Release SDA

    for (int j = 0; j < 8; j++) {
        clockLineHigh();
        IR_DELAY();
        byte <<= 1;
        if (sampleSDA()) byte |= 1;
        clockLineLow(); IR_DELAY();
    }

    // Send ACK/NACK
    if (ack) dataLineLow();
    else     dataLineHigh();
    IR_DELAY();
    clockLineHigh(); IR_DELAY();
    clockLineLow(); IR_DELAY();
    dataLineHigh();  // Release SDA

    return byte;
}

// === Public Functions ===

void IR_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    // Set both SDA and SCL as input initially
    GPIOB->MODER &= ~((3 << (2 * IR_SDA)) | (3 << (2 * IR_SCL)));
    GPIOB->PUPDR &= ~((3 << (2 * IR_SDA)) | (3 << (2 * IR_SCL)));  // No pull
    GPIOB->OTYPER |= (1 << IR_SDA) | (1 << IR_SCL);               // Open-drain
    GPIOB->OSPEEDR |= (3 << (2 * IR_SDA)) | (3 << (2 * IR_SCL));  // High speed
    GPIOB->ODR |= (1 << IR_SDA) | (1 << IR_SCL);                  // Pull up externally

    dataLineHigh();
    clockLineHigh();
}

uint16_t IR_ReadRaw(void) {
    uint8_t high = 0, low = 0;

    startCondition();
    sendByte(SENSOR_ID << 1);   // Write address
    sendByte(0x01);             // Temperature register
    startCondition();
    sendByte((SENSOR_ID << 1) | 0x01); // Read address
    high = readByte(1);
    low  = readByte(0);
    stopCondition();

    return ((uint16_t)high << 8) | low;
}

float IR_ReadTemperature(void) {
    return ((float)IR_ReadRaw()) * 0.1f;
}

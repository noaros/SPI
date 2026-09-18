#include <stdio.h>
#include <sys/stat.h>
#include "stm32f429xx.h"
#include <stdint.h>



// CS - PA4
// SCLK - PA5
// MISO - PA6
// MOSI - PA7

void _init(void) {}
int _close(int fd) {}
int _lseek(int fd, int ptr, int dir) {}
int _read(int fd, char *ptr, int len) {}
int _fstat(int fd, struct stat *st) {}
int _isatty(int fd) {}

void SystemInit(void) {
	SysTick_Config(16000000);
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
}

void *_sbrk(int incr) {
  extern char _end;
  static unsigned char *heap = NULL;
  unsigned char *prev_heap;
  if (heap == NULL) heap = (unsigned char *) &_end;
  prev_heap = heap;
  heap += incr;
  return prev_heap;
}

int _write(int file, char *ptr, int len) {
	while (len--) {
		USART3->DR = *ptr++;
		while (!(USART3->SR & USART_SR_TXE)) {}
	}
}

// BMI160 Register Definitions
#define BMI160_CHIP_ID_ADDR  0x00
#define BMI160_EXPECTED_ID   0xD1
#define BMI160_SPI_READ_BIT  0x80

// CS Pin Helpers (PA4)
#define CS_LOW()   (GPIOA->BSRR = GPIO_BSRR_BR4)
#define CS_HIGH()  (GPIOA->BSRR = GPIO_BSRR_BS4)

void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 4000; i++) {
        __NOP();
    }
}

void spi1_init(void) {
    // 1. Enable Clocks for GPIOA and SPI1
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    // 2. Configure PA4 as GPIO Output (Chip Select)
    GPIOA->MODER &= ~(GPIO_MODER_MODE4);
    GPIOA->MODER |=  (1U << GPIO_MODER_MODE4_Pos); // General Purpose Output

    // 3. Configure PA5 (SCK), PA6 (MISO), PA7 (MOSI) as Alternate Function (AF5)
    GPIOA->MODER &= ~(GPIO_MODER_MODE5 | GPIO_MODER_MODE6 | GPIO_MODER_MODE7);
    GPIOA->MODER |=  (2U << GPIO_MODER_MODE5_Pos) | 
                     (2U << GPIO_MODER_MODE6_Pos) | 
                     (2U << GPIO_MODER_MODE7_Pos);

    // Set AF5 (SPI1) in Alternate Function Low Register for Pins 5, 6, 7
    GPIOA->AFR[0] &= ~((0xFU << GPIO_AFRL_AFSEL5_Pos) | 
                       (0xFU << GPIO_AFRL_AFSEL6_Pos) | 
                       (0xFU << GPIO_AFRL_AFSEL7_Pos));
                       
    GPIOA->AFR[0] |=  (5U << GPIO_AFRL_AFSEL5_Pos) | 
                       (5U << GPIO_AFRL_AFSEL6_Pos) | 
                       (5U << GPIO_AFRL_AFSEL7_Pos);

    // Set High Speed for Pins PA4, PA5, PA6, PA7
    GPIOA->OSPEEDR |= (3U << GPIO_OSPEEDR_OSPEED4_Pos) | 
                      (3U << GPIO_OSPEEDR_OSPEED5_Pos) | 
                      (3U << GPIO_OSPEEDR_OSPEED6_Pos) | 
                      (3U << GPIO_OSPEEDR_OSPEED7_Pos);

    // Enable Pull-up on MISO (PA6)
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD6);
    GPIOA->PUPDR |=  (1U << GPIO_PUPDR_PUPD6_Pos);

    // Deselect peripheral
    CS_HIGH();

    // 4. Configure SPI1 Peripheral
    // Master Mode, Software Slave Management (SSM/SSI set), Clock Divider = f_PCLK/16
    SPI1->CR1 = SPI_CR1_MSTR |
                SPI_CR1_SSM  |
                SPI_CR1_SSI  |
                (3U << SPI_CR1_BR_Pos); // BR[2:0] = 011 -> PCLK/16

    // Enable SPI Peripheral
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t spi1_transfer(uint8_t data) {
    // Wait until Transmit Buffer is Empty (TXE)
    while (!(SPI1->SR & SPI_SR_TXE));

    // Send Data
    *(volatile uint8_t *)&SPI1->DR = data;

    // Wait until Receive Buffer contains data (RXNE)
    while (!(SPI1->SR & SPI_SR_RXNE));

    // Read Data
    return *(volatile uint8_t *)&SPI1->DR;
}

uint8_t bmi160_read_chip_id(void) {
    uint8_t chip_id = 0;

    CS_LOW();

    // Transmit register address with bit 7 set for read
    spi1_transfer(BMI160_CHIP_ID_ADDR | BMI160_SPI_READ_BIT);

    // Clock out dummy byte to read chip ID payload
    chip_id = spi1_transfer(0x00);

    CS_HIGH();

    return chip_id;
}

void main() {
	RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
	GPIOD->MODER |= GPIO_MODER_MODER8_1;
	GPIOD->MODER |= GPIO_MODER_MODER9_1;
	GPIOD->AFR[1] |= (7 << GPIO_AFRH_AFSEL8_Pos) | (7 << GPIO_AFRH_AFSEL9_Pos);
	USART3->BRR = 16000000 / 115200;
	USART3->CR1 |= (USART_CR1_UE | USART_CR1_TE);

    *(volatile int *)0x20000004=0xaaaa;


    spi1_init();

    // Force BMI160 into SPI mode by toggling CS line
    CS_HIGH();
    delay_ms(1);
    CS_LOW();
    delay_ms(1);
    CS_HIGH();
    delay_ms(10); // Latching delay for SPI mode init

    uint8_t chip_id = bmi160_read_chip_id();

    *(volatile unsigned int *)0x20000004=chip_id;

again:
	*(volatile int *)0x20000000 += 1;//heartbeat

	
    printf("chip_id %d\r\n",chip_id);
	// printf("Hello %d %d %d\r\n",accX,accY,accZ);
wait:
	while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)) {}
	goto again;
}

//////



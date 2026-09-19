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

#define BMI160_CHIP_ID_ADDR  0x00
#define BMI160_EXPECTED_ID   0xD1
#define BMI160_SPI_READ_BIT  0x80
#define BMI160_ACC_DATA_ADDR   0x12   // Start register for Acc X (LSB)
#define BMI160_CMD_ADDR        0x7E   // Command Register
#define BMI160_CMD_ACC_MODE_NORMAL 0x11 

// CS Pin Helpers (PA4)
#define CS_LOW()   (GPIOA->BSRR = GPIO_BSRR_BR4)
#define CS_HIGH()  (GPIOA->BSRR = GPIO_BSRR_BS4)

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t z;
} acc_t;

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

// Write a byte to a BMI160 register over SPI
void bmi160_write_register(uint8_t reg, uint8_t value) {
    CS_LOW();
    spi1_transfer(reg & ~BMI160_SPI_READ_BIT); // Clear bit 7 for Write
    spi1_transfer(value);
    CS_HIGH();
}

void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 4000; i++) {
        __NOP();
    }
}

// Power up the Accelerometer into Normal Mode
void bmi160_accel_init(void) {
    // Write command 0x11 to register 0x7E
    bmi160_write_register(BMI160_CMD_ADDR, BMI160_CMD_ACC_MODE_NORMAL);
    
    // Delay required for Accel power-up (~3.8ms according to BMI160 datasheet)
    delay_ms(10);
}

// Read raw X, Y, Z accelerometer data
void bmi160_read_accel(acc_t *accel) {
    uint8_t raw_buffer[6];

    CS_LOW();
    
    // Address with bit 7 set for Read
    spi1_transfer(BMI160_ACC_DATA_ADDR | BMI160_SPI_READ_BIT);
    
    // Burst-read 6 sequential bytes: [X_LSB, X_MSB, Y_LSB, Y_MSB, Z_LSB, Z_MSB]
    for (int i = 0; i < 6; i++) {
        raw_buffer[i] = spi1_transfer(0x00);
    }
    
    CS_HIGH();

    // Combine LSB and MSB into signed 16-bit integers
    accel->x = (int16_t)((raw_buffer[1] << 8) | raw_buffer[0]);
    accel->y = (int16_t)((raw_buffer[3] << 8) | raw_buffer[2]);
    accel->z = (int16_t)((raw_buffer[5] << 8) | raw_buffer[4]);
}


void spi1_init(void) {
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

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    GPIOA->MODER |=  1 << GPIO_MODER_MODE4_Pos; //PA4 output
    // 3. Configure PA5 (SCK), PA6 (MISO), PA7 (MOSI) as Alternate Function
    GPIOA->MODER |=  2 << GPIO_MODER_MODE5_Pos | 2 << GPIO_MODER_MODE6_Pos | 2 << GPIO_MODER_MODE7_Pos;
    // Set them to AF5 (SPI1)                       
    GPIOA->AFR[0] |=  5 << GPIO_AFRL_AFSEL5_Pos | 5 << GPIO_AFRL_AFSEL6_Pos | 5 << GPIO_AFRL_AFSEL7_Pos;
    // In some other context might need to set 'high speed', maybe use internal pullups for MISO or CS
    CS_HIGH();//deselects sensor while we configure spi (not actually needed per my testing), but also puts sensor into SPI mode
    // Master Mode, Software Slave Management (SSM/SSI set), Clock Divider = f_PCLK/16
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM  | SPI_CR1_SSI  | 3 << SPI_CR1_BR_Pos;
    SPI1->CR1 |= SPI_CR1_SPE;// enable

    delay_ms(1); // Latching delay for SPI mode init

    uint8_t chip_id = bmi160_read_chip_id();

    *(volatile unsigned int *)0x20000004=chip_id;

    bmi160_accel_init();
again:
	*(volatile int *)0x20000000 -= 1;//heartbeat

	
    printf("chip_id %d\r\n",chip_id);
    acc_t acc;

    bmi160_read_accel(&acc);
	printf("Hello %d %d %d\r\n",acc.x,acc.y,acc.z);
wait:
	while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)) {}
	goto again;
}

//////



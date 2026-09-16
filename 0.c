#include <stdio.h>
#include <sys/stat.h>
#include "stm32f429xx.h"

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

void main() {
	RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
	GPIOD->MODER |= GPIO_MODER_MODER8_1;
	GPIOD->MODER |= GPIO_MODER_MODER9_1;
	GPIOD->AFR[1] |= (7 << GPIO_AFRH_AFSEL8_Pos) | (7 << GPIO_AFRH_AFSEL9_Pos);
	USART3->BRR = 16000000 / 115200;
	USART3->CR1 |= (USART_CR1_UE | USART_CR1_TE);


again:
	*(volatile int *)0x20000000 += 1;//heartbeat
	printf("Hello \r\n");
	while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)) {}
	goto again;
}


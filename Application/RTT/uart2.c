/**
 * @file    uart2.c
 * @brief   USART2 阻塞发送 + RX 中断环形缓冲（PA2 TX / PA3 RX）
 */
#include "uart2.h"
#include <stddef.h>
#include "misc.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"

#define UART2_RX_BUF_SIZE 256U
#define UART2_IRQ_PRIORITY 6U

static uint8_t s_rx_buf[UART2_RX_BUF_SIZE];
static volatile uint16_t s_rx_head;
static volatile uint16_t s_rx_tail;

uint8_t uart2_init(uint32_t baudrate)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	if (baudrate == 0U)
		return 1U;

	s_rx_head = 0U;
	s_rx_tail = 0U;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = baudrate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART2, &USART_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = UART2_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0U;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART2, ENABLE);

	return 0U;
}

void uart2_write(const uint8_t *pBuffer, uint32_t size)
{
	uint32_t i;

	if ((pBuffer == NULL) || (size == 0U))
		return;

	for (i = 0U; i < size; i++) {
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
			;
		USART_SendData(USART2, (uint16_t)pBuffer[i]);
	}
}

uint32_t uart2_read(uint8_t *pBuffer, uint32_t size)
{
	uint32_t n;
	uint16_t head;
	uint16_t tail;

	if ((pBuffer == NULL) || (size == 0U))
		return 0U;

	n = 0U;
	tail = s_rx_tail;
	head = s_rx_head;
	while ((n < size) && (tail != head)) {
		pBuffer[n] = s_rx_buf[tail];
		n++;
		tail++;
		if (tail >= UART2_RX_BUF_SIZE)
			tail = 0U;
		head = s_rx_head;
	}
	s_rx_tail = tail;
	return n;
}

void USART2_IRQHandler(void)
{
	uint8_t ch;
	uint16_t head;
	uint16_t next;

	if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET) {
		ch = (uint8_t)USART_ReceiveData(USART2);
		head = s_rx_head;
		next = head + 1U;
		if (next >= UART2_RX_BUF_SIZE)
			next = 0U;
		if (next != s_rx_tail) {
			s_rx_buf[head] = ch;
			s_rx_head = next;
		}
	}

	if (USART_GetFlagStatus(USART2, USART_FLAG_ORE) == SET)
		(void)USART_ReceiveData(USART2);
}

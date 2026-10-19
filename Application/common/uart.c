/**
 * @file    uart.c
 * @brief   USART2 / USART3 驱动（CDC 桥接，两路独立，可单独注释）
 *
 * TX：uart2_write / uart3_write 写入 stream buffer 后切发送方向并打开 TXE，
 *     ISR 逐字节取出写入 DR；缓冲空则改等 TC，移位完成后再切回收。
 * RX：ISR 写入 stream buffer，任务取出后交给应用层。
 */
#include "uart.h"

#include "misc.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"

#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "string.h"
#include "task.h"

#define UART_IRQ_PRIORITY 6U
#define UART_RX_BUF_SIZE 1024U
#define UART_TX_BUF_SIZE 1024U
#define UART_DEFAULT_BAUD 115200U

#define UART_TASK_STACK 128U
#define UART_TASK_PRIO 2U

static uart_config_t s_uart2_config_current;
static uart_config_t s_uart2_config = {
	.baudrate = UART_DEFAULT_BAUD,
	.data_bits = 8U,
	.stop_bits = 0U,
	.parity = 0U,
};
static uint8_t s_uart2_buf[UART_TX_BUF_SIZE];
static StreamBufferHandle_t s_uart2_rx_buffer;
static StreamBufferHandle_t s_uart2_tx_buffer;

static uart_config_t s_uart3_config_current;
static uart_config_t s_uart3_config = {
	.baudrate = UART_DEFAULT_BAUD,
	.data_bits = 8U,
	.stop_bits = 0U,
	.parity = 0U,
};
static uint8_t s_uart3_buf[UART_TX_BUF_SIZE];
static StreamBufferHandle_t s_uart3_rx_buffer;
static StreamBufferHandle_t s_uart3_tx_buffer;

static void uart2_hw_init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = UART_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

static void uart3_hw_init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = UART_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

static void uart_config_apply(USART_TypeDef *usart, const uart_config_t *config) {
	USART_InitTypeDef USART_InitStructure;

	if ((usart == NULL) || (config == NULL))
		return;

	USART_Cmd(usart, DISABLE);
	USART_StructInit(&USART_InitStructure);

	USART_InitStructure.USART_BaudRate = config->baudrate;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	if (config->parity == 1U) {
		USART_InitStructure.USART_Parity = USART_Parity_Odd;
		USART_InitStructure.USART_WordLength = USART_WordLength_9b;
	} else if (config->parity == 2U) {
		USART_InitStructure.USART_Parity = USART_Parity_Even;
		USART_InitStructure.USART_WordLength = USART_WordLength_9b;
	} else {
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_WordLength = (config->data_bits == 9U) ? USART_WordLength_9b : USART_WordLength_8b;
	}

	if (config->stop_bits == 2U)
		USART_InitStructure.USART_StopBits = USART_StopBits_2;
	else if (config->stop_bits == 1U)
		USART_InitStructure.USART_StopBits = USART_StopBits_1_5;
	else
		USART_InitStructure.USART_StopBits = USART_StopBits_1;

	USART_Init(usart, &USART_InitStructure);
	USART_Cmd(usart, ENABLE);
	USART_ITConfig(usart, USART_IT_RXNE, ENABLE);
}

void uart2_config_set(const uart_config_t *config) {
	if (config == NULL)
		return;
	memcpy(&s_uart2_config, config, sizeof(uart_config_t));
}

void uart2_config_get(uart_config_t *config) {
	if (config == NULL)
		return;
	memcpy(config, &s_uart2_config, sizeof(uart_config_t));
}

void uart2_write(const uint8_t *pBuffer, size_t size) {
	if ((pBuffer == NULL) || (size == 0U) || (s_uart2_tx_buffer == NULL))
		return;
	xStreamBufferSend(s_uart2_tx_buffer, (void *)pBuffer, size, pdMS_TO_TICKS(10));
	uart2_dir_set(1U);
	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
}

__WEAK void uart2_read(const uint8_t *pBuffer, size_t size) {
	(void)pBuffer;
	(void)size;
}

__WEAK void uart2_dir_set(uint8_t is_tx) {
	(void)is_tx;
}

static void uart2_task(void *param) {
	(void)param;
	while (1) {
		size_t xBytesReceived;

		xBytesReceived = xStreamBufferReceive(s_uart2_rx_buffer, s_uart2_buf, UART_RX_BUF_SIZE, pdMS_TO_TICKS(10));
		if (xBytesReceived > 0)
			uart2_read(s_uart2_buf, xBytesReceived);

		if ((s_uart2_config_current.baudrate != s_uart2_config.baudrate) || (s_uart2_config_current.data_bits != s_uart2_config.data_bits) ||
			(s_uart2_config_current.stop_bits != s_uart2_config.stop_bits) || (s_uart2_config_current.parity != s_uart2_config.parity)) {
			USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
			USART_ITConfig(USART2, USART_IT_TC, DISABLE);
			USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
			uart_config_apply(USART2, &s_uart2_config);
			memcpy(&s_uart2_config_current, &s_uart2_config, sizeof(uart_config_t));
			uart2_dir_set(0U);
		}

		if (xStreamBufferBytesAvailable(s_uart2_tx_buffer) > 0U) {
			uart2_dir_set(1U);
			USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
		}
	}
}

void uart2_init(void) {
	s_uart2_rx_buffer = xStreamBufferCreate(UART_RX_BUF_SIZE, 1U);
	s_uart2_tx_buffer = xStreamBufferCreate(UART_TX_BUF_SIZE, 1U);
	uart2_hw_init();
	uart_config_apply(USART2, &s_uart2_config);
	memcpy(&s_uart2_config_current, &s_uart2_config, sizeof(uart_config_t));
	uart2_dir_set(0U);
	xTaskCreate(uart2_task, "uart2", UART_TASK_STACK, NULL, UART_TASK_PRIO, NULL);
}

void uart3_config_set(const uart_config_t *config) {
	if (config == NULL)
		return;
	memcpy(&s_uart3_config, config, sizeof(uart_config_t));
}

void uart3_config_get(uart_config_t *config) {
	if (config == NULL)
		return;
	memcpy(config, &s_uart3_config, sizeof(uart_config_t));
}

void uart3_write(const uint8_t *pBuffer, size_t size) {
	if ((pBuffer == NULL) || (size == 0U) || (s_uart3_tx_buffer == NULL))
		return;
	xStreamBufferSend(s_uart3_tx_buffer, (void *)pBuffer, size, pdMS_TO_TICKS(10));
	uart3_dir_set(1U);
	USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
}

__WEAK void uart3_read(const uint8_t *pBuffer, size_t size) {
	(void)pBuffer;
	(void)size;
}

__WEAK void uart3_dir_set(uint8_t is_tx) {
	(void)is_tx;
}

static void uart3_task(void *param) {
	(void)param;
	while (1) {
		size_t xBytesReceived;

		xBytesReceived = xStreamBufferReceive(s_uart3_rx_buffer, s_uart3_buf, UART_RX_BUF_SIZE, pdMS_TO_TICKS(10));
		if (xBytesReceived > 0)
			uart3_read(s_uart3_buf, xBytesReceived);

		if ((s_uart3_config_current.baudrate != s_uart3_config.baudrate) || (s_uart3_config_current.data_bits != s_uart3_config.data_bits) ||
			(s_uart3_config_current.stop_bits != s_uart3_config.stop_bits) || (s_uart3_config_current.parity != s_uart3_config.parity)) {
			USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
			USART_ITConfig(USART3, USART_IT_TC, DISABLE);
			USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
			uart_config_apply(USART3, &s_uart3_config);
			memcpy(&s_uart3_config_current, &s_uart3_config, sizeof(uart_config_t));
			uart3_dir_set(0U);
		}

		if (xStreamBufferBytesAvailable(s_uart3_tx_buffer) > 0U) {
			uart3_dir_set(1U);
			USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
		}
	}
}

void uart3_init(void) {
	s_uart3_rx_buffer = xStreamBufferCreate(UART_RX_BUF_SIZE, 1U);
	s_uart3_tx_buffer = xStreamBufferCreate(UART_TX_BUF_SIZE, 1U);
	uart3_hw_init();
	uart_config_apply(USART3, &s_uart3_config);
	memcpy(&s_uart3_config_current, &s_uart3_config, sizeof(uart_config_t));
	uart3_dir_set(0U);
	xTaskCreate(uart3_task, "uart3", UART_TASK_STACK, NULL, UART_TASK_PRIO, NULL);
}

void USART2_IRQHandler(void) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	uint8_t ch;

	if (SET == USART_GetITStatus(USART2, USART_IT_RXNE)) {
		ch = (uint8_t)USART_ReceiveData(USART2);
		if (s_uart2_rx_buffer != NULL)
			xStreamBufferSendFromISR(s_uart2_rx_buffer, &ch, 1U, &xHigherPriorityTaskWoken);
	}

	if (SET == USART_GetITStatus(USART2, USART_IT_TXE)) {
		if (xStreamBufferReceiveFromISR(s_uart2_tx_buffer, &ch, 1U, &xHigherPriorityTaskWoken) > 0)
			USART_SendData(USART2, (uint16_t)ch);
		else {
			USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
			USART_ClearFlag(USART2, USART_FLAG_TC);
			USART_ITConfig(USART2, USART_IT_TC, ENABLE);
		}
	}

	if (SET == USART_GetITStatus(USART2, USART_IT_TC)) {
		USART_ITConfig(USART2, USART_IT_TC, DISABLE);
		if ((USART2->CR1 & USART_CR1_TXEIE) == 0U) {
			if (xStreamBufferBytesAvailable(s_uart2_tx_buffer) > 0U)
				USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
			else
				uart2_dir_set(0U);
		}
	}

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void USART3_IRQHandler(void) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	uint8_t ch;

	if (SET == USART_GetITStatus(USART3, USART_IT_RXNE)) {
		ch = (uint8_t)USART_ReceiveData(USART3);
		if (s_uart3_rx_buffer != NULL)
			xStreamBufferSendFromISR(s_uart3_rx_buffer, &ch, 1U, &xHigherPriorityTaskWoken);
	}

	if (SET == USART_GetITStatus(USART3, USART_IT_TXE)) {
		if (xStreamBufferReceiveFromISR(s_uart3_tx_buffer, &ch, 1U, &xHigherPriorityTaskWoken) > 0)
			USART_SendData(USART3, (uint16_t)ch);
		else {
			USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
			USART_ClearFlag(USART3, USART_FLAG_TC);
			USART_ITConfig(USART3, USART_IT_TC, ENABLE);
		}
	}

	if (SET == USART_GetITStatus(USART3, USART_IT_TC)) {
		USART_ITConfig(USART3, USART_IT_TC, DISABLE);
		if ((USART3->CR1 & USART_CR1_TXEIE) == 0U) {
			if (xStreamBufferBytesAvailable(s_uart3_tx_buffer) > 0U)
				USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
			else
				uart3_dir_set(0U);
		}
	}

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

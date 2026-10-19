/**
 * @file    main.c
 * @brief   DEMO：LED 闪烁 + RTT / SWO / USART2 打印
 *
 * SWO：PB3(TRACESWO)
 *   - ST-Link：仿真器 13 脚 ←→ PB3（launch.json swoConfig）
 *   - DAPLink：PA10 ←→ PB3
 * UART：USART2 PA2 TX / PA3 RX，115200 8N1
 *   USB-TTL RX ← PA2，USB-TTL TX → PA3，共地
 *   收到的数据在任务里 printf（RTT / SWO / 串口）
 */
#include "misc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#include "stdio.h"
#include "stdint.h"

#include "FreeRTOS.h"
#include "task.h"


// jtag 模式下无法使用 SWO 打印，只能使用 RTT 打印
// swd 模式下可以同时使用 RTT 和 SWO 打印
// jtag 连不上时，检查上次是否是 swd 模式，且开启了SWO功能,SWO会占用PB3引脚，导致无法连接JTAG

#define PRINT_SWO  1
#define PRINT_RTT  1
#define PRINT_UART 1

#include "rtt.h"
#include "swo.h"
#include "uart2.h"

#define SWO_BAUDRATE_HZ  1000000U
#define UART_BAUDRATE_HZ 115200U
#define UART_RX_LINE_MAX 128U

static TaskHandle_t xHandle1 = NULL;


int _write(int fd, char *pBuffer, int size)
{
	(void)fd;
	if ((pBuffer == NULL) || (size <= 0))
		return -1;

#if PRINT_RTT
	(void)rtt_write(0U, pBuffer, (uint32_t)size);
#endif
#if PRINT_SWO
	(void)swo_write(pBuffer, (uint32_t)size);
#endif
#if PRINT_UART
	uart2_write((const uint8_t *)pBuffer, (uint32_t)size);
#endif
	return size;
}

#if PRINT_UART
static void uart_rx_line_print(char *pLine, uint32_t *pLen)
{
	if ((pLine == NULL) || (pLen == NULL) || (*pLen == 0U))
		return;

	pLine[*pLen] = '\0';
	printf("RX: %s\r\n", pLine);
	*pLen = 0U;
}

static void vTaskUartRx(void *param)
{
	uint8_t chunk[64];
	char line[UART_RX_LINE_MAX];
	uint32_t line_len = 0U;
	uint32_t idle_cnt = 0U;
	uint32_t n;
	uint32_t i;
	uint8_t ch;

	(void)param;
	while (1) {
		n = uart2_read(chunk, (uint32_t)sizeof(chunk));
		if (n == 0U) {
			if ((line_len > 0U) && (idle_cnt >= 5U))
				uart_rx_line_print(line, &line_len);
			else if (line_len > 0U)
				idle_cnt++;
			vTaskDelay(pdMS_TO_TICKS(10));
			continue;
		}

		idle_cnt = 0U;
		for (i = 0U; i < n; i++) {
			ch = chunk[i];
			if ((ch == (uint8_t)'\n') || (ch == (uint8_t)'\r')) {
				uart_rx_line_print(line, &line_len);
			} else if (line_len < (UART_RX_LINE_MAX - 1U)) {
				line[line_len] = (char)ch;
				line_len++;
			} else {
				uart_rx_line_print(line, &line_len);
				line[0] = (char)ch;
				line_len = 1U;
			}
		}
	}
}
#endif

static void vTask1(void *param)
{
	uint32_t count = 0U;

	(void)param;
	while (1) {
		vTaskDelay(pdMS_TO_TICKS(1000));
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
		printf("led on  %lu\r\n", (unsigned long)count);

		vTaskDelay(pdMS_TO_TICKS(1000));
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		printf("led off %lu\r\n", (unsigned long)count);
		count++;
	}
}

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	GPIO_InitTypeDef GPIO_InitStruct = {
		.GPIO_Pin = GPIO_Pin_13,
		.GPIO_Mode = GPIO_Mode_Out_PP,
		.GPIO_Speed = GPIO_Speed_50MHz};
	GPIO_Init(GPIOC, &GPIO_InitStruct);

#if PRINT_UART
	uart2_init(UART_BAUDRATE_HZ);
	printf("UART ready, %lu 8N1 PA2\r\n", (unsigned long)UART_BAUDRATE_HZ);
#endif
#if PRINT_RTT
	rtt_init();
	printf("RTT ready\r\n");
#endif
#if PRINT_SWO
	swo_init(SystemCoreClock, SWO_BAUDRATE_HZ);
	printf("SWO ready, swo=%lu Hz\r\n", (unsigned long)SWO_BAUDRATE_HZ);
#endif
	

	xTaskCreate(vTask1, "vTask1", 256, NULL, 4, &xHandle1);
#if PRINT_UART
	xTaskCreate(vTaskUartRx, "uart_rx", 256, NULL, 3, NULL);
#endif
	vTaskStartScheduler();
}

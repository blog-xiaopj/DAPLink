/**
 * @file    main.c
 * @brief   CMSIS-DAP v2（WinUSB）+ 两路 CDC ACM 虚拟串口应用入口
 */
#include "misc.h"
#include "stdio.h"

#include "FreeRTOS.h"
#include "task.h"

#include "uart.h"

#include "usbd_core.h"

#include "usbd_cdc_acm.h"
#include "usb_dap.h"

void uart2_read(const uint8_t *pBuffer, size_t size)
{
	usbd_cdc_acm_transfer((const char *)pBuffer, size);
}

void usbd_cdc_acm_received(const char *pBuffer, size_t size)
{
	uart2_write((const uint8_t *)pBuffer, size);
}

void uart3_read(const uint8_t *pBuffer, size_t size)
{
	usbd_cdc_acm2_transfer((const char *)pBuffer, size);
}

void usbd_cdc_acm2_received(const char *pBuffer, size_t size)
{
	uart3_write((const uint8_t *)pBuffer, size);
}

void uart3_dir_set(uint8_t is_tx) {
	if (is_tx)
		GPIO_SetBits(GPIOB, GPIO_Pin_1);
	else
		GPIO_ResetBits(GPIOB, GPIO_Pin_1);
}

void usbd_cdc_acm_set_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
	uart_config_t config = {
		.baudrate = line_coding->dwDTERate,
		.data_bits = line_coding->bDataBits,
		.stop_bits = line_coding->bCharFormat,
		.parity = line_coding->bParityType,
	};

	(void)busid;
	if (intf >= 3U)
		uart3_config_set(&config);
	else
		uart2_config_set(&config);
}

void usbd_cdc_acm_get_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
	uart_config_t config;

	(void)busid;
	if (intf >= 3U)
		uart3_config_get(&config);
	else
		uart2_config_get(&config);
	line_coding->dwDTERate = config.baudrate;
	line_coding->bDataBits = config.data_bits;
	line_coding->bCharFormat = config.stop_bits;
	line_coding->bParityType = config.parity;
}

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	// 初始化RS485方向引脚
	{
		GPIO_InitTypeDef GPIO_InitStructure;
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOB, &GPIO_InitStructure);
	}

	uart2_init();
	uart3_init();
	usb_dap_init();

	vTaskStartScheduler();

	while (1) {
	}
}

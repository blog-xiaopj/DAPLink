/**
 * @file    usb_port.c
 * @brief   STM32F103 USB FS 底层初始化（时钟、NVIC、D+ 断开脉冲）
 */
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"

#include "usbd_core.h"

#define USB_DISCONNECT_DELAY_MS 50U
#define USB_IRQ_PRIORITY        6U

/**
 * @brief 粗略毫秒忙等（调度器启动前使用）
 * @param ms 延时毫秒数
 */
static void usb_delay_ms(uint32_t ms)
{
	volatile uint32_t ticks;

	ticks = (SystemCoreClock / 8000U) * ms;
	while (ticks > 0U) {
		ticks--;
	}
}

/**
 * @brief 使能 USB 48 MHz 时钟、NVIC，并对 PA12 做断开脉冲
 */
void usb_dc_low_level_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/* PA12(D+) 拉低，让主机看到断开后再枚举 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA, GPIO_Pin_12);
	usb_delay_ms(USB_DISCONNECT_DELAY_MS);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* SYSCLK=72MHz 时 USBCLK = PLL/1.5 = 48MHz */
	RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USB_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/**
 * @brief 关闭 USB 时钟与中断
 */
void usb_dc_low_level_deinit(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USB_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
	NVIC_Init(&NVIC_InitStructure);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, DISABLE);
}

/**
 * @brief USB 低优先级中断
 */
void USB_LP_CAN1_RX0_IRQHandler(void)
{
	USBD_IRQHandler(0);
}

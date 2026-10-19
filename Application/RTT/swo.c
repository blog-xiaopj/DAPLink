/**
 * @file    swo.c
 * @brief   STM32F103 ITM/SWO UART（NRZ）打印，PB3 = TRACESWO
 *
 * 接线：
 *   - ST-Link 仿真：仿真器 13 脚(SWO) ←→ 本板 PB3，共地
 *   - DAPLink 抓包：DAP PA10 ←→ 本板 PB3，共地
 */
#include "swo.h"

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#include <stddef.h>

#define SWO_ITM_PORT      0U
#define SWO_LAR_KEY       0xC5ACCE55UL
#define SWO_ITM_TCR_ATBID (1UL << 16)
#define SWO_TX_TIMEOUT    10000U

static void swo_itm_enable(uint32_t cpu_hz, uint32_t swo_hz)
{
	uint32_t prescaler;

	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

	prescaler = (cpu_hz / swo_hz) - 1U;
	TPI->ACPR = prescaler;
	TPI->SPPR = 2U; /* NRZ UART */
	TPI->FFCR = 0x100U;

	ITM->LAR = SWO_LAR_KEY;
	ITM->TCR = 0U;
	ITM->TER = 0U;
	ITM->TPR = 0U;
	ITM->TER = (1UL << SWO_ITM_PORT);
	ITM->TCR = ITM_TCR_ITMENA_Msk | ITM_TCR_SWOENA_Msk | ITM_TCR_SYNCENA_Msk | SWO_ITM_TCR_ATBID;
}

static void swo_pin_enable(void)
{
	/* 关 JTAG、保留 SWD，释放 PB3 给 TRACESWO */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	/* 异步 TRACE：TRACE_MODE=00，使能 TRACE_IOEN → PB3 */
	DBGMCU->CR &= ~DBGMCU_CR_TRACE_MODE;
	DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;
}

uint8_t swo_init(uint32_t cpu_hz, uint32_t swo_hz)
{
	if ((cpu_hz == 0U) || (swo_hz == 0U) || (swo_hz > cpu_hz))
		return 1U;

	/* 始终打开 PB3，供 ST-Link SWO / DAPLink 抓取 */
	swo_pin_enable();
	swo_itm_enable(cpu_hz, swo_hz);

	return 0U;
}

void swo_putchar(uint8_t ch)
{
	uint32_t timeout;

	if (((ITM->TCR & ITM_TCR_ITMENA_Msk) == 0U) || ((ITM->TER & (1UL << SWO_ITM_PORT)) == 0U))
		return;

	/* 非阻塞：FIFO 满则丢弃，避免仿真时卡死 */
	timeout = SWO_TX_TIMEOUT;
	while ((ITM->PORT[SWO_ITM_PORT].u32 == 0U) && (timeout != 0U))
		timeout--;

	if (timeout != 0U)
		ITM->PORT[SWO_ITM_PORT].u8 = ch;
}

uint32_t swo_write(const void *pBuffer, uint32_t size)
{
	const uint8_t *pSrc = (const uint8_t *)pBuffer;
	uint32_t i;

	if ((pBuffer == NULL) || (size == 0U))
		return 0U;

	for (i = 0U; i < size; i++)
		swo_putchar(pSrc[i]);

	return size;
}

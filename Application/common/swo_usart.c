/**
 * @file    swo_usart.c
 * @brief   CMSIS-Driver USART1 仅接收驱动（PA10）+ DMA1 Ch5，供 CMSIS-DAP SWO
 * @author  DAPLink
 * @date    2026-09
 *
 * @par 硬件连接
 *   目标板 SWO / TRACESWO ──→ 本板 PA10（USART1_RX）
 *   PCB 短接 PA10(SWO) 与 PA6(JTDO)，再接到调试座 TDO/SWO（ST-Link 式）
 *   共地；不占用 PA9（USART1_TX），本驱动仅开 RX。
 *
 * @par 数据路径
 *   USART1 DR ──DMA1 Channel5──→ SWO.c TraceBuf
 *   DMA 传输完成 / USART 错误 ──回调──→ SWO.c USART_Callback
 *
 * @par 实现范围
 *   仅实现 SWO.c 实际调用的 CMSIS-Driver API；
 *   Send / Transfer / Modem / 同步模式返回 UNSUPPORTED。
 *
 * @par 中断与 RTOS
 *   DMA1_Ch5 / USART1 抢占优先级 = 4
 *   （高于 USB/CDC UART=6，且高于 configLIBRARY_MAX_SYSCALL=5）
 *   ISR 内不得调用 FreeRTOS API；s_cb_event（SWO 回调）也不调用。
 *
 * @par 驱动状态机（s_flags）
 * @code
 *   Initialize ──► FLG_INIT
 *       │
 *       ▼ PowerControl(FULL)
 *   FLG_POWER（时钟/GPIO/NVIC）
 *       │
 *       ▼ Control(ASYNCHRONOUS|8N1, baud)
 *   FLG_CFG（USART 已配置）
 *       │
 *       ▼ Control(CONTROL_RX, 1) + Receive()
 *   FLG_RX_EN + s_rx_busy（DMA 收数中）
 * @endcode
 */

#include "Driver_USART.h"

#include "misc.h"
#include "stm32f10x.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"

/** DMA / USART 中断抢占优先级（数值越小优先级越高） */
#define SWO_USART_IRQ_PRIORITY 4U

/**
 * @name s_flags 位定义
 * @brief 驱动生命周期标志，按位或组合存放于 s_flags
 *
 * | 宏        | 位  | 掩码 | 含义 |
 * |-----------|-----|------|------|
 * | FLG_INIT  | 0   | 0x01 | Initialize() 已调用，回调已保存 |
 * | FLG_POWER | 1   | 0x02 | PowerControl(FULL) 已上电：时钟/GPIO/NVIC 就绪 |
 * | FLG_CFG   | 2   | 0x04 | Control(ASYNCHRONOUS) 已配置波特率与 8N1 |
 * | FLG_RX_EN | 3   | 0x08 | Control(CONTROL_RX,1) 允许接收；DMA 请求可打开 |
 *
 * 位 4..7 保留，清零。
 *
 * 典型合法组合：
 *   0x01          仅初始化
 *   0x03          已上电（INIT|POWER）
 *   0x07          已配置（INIT|POWER|CFG），可 Receive
 *   0x0F          接收使能（含 RX_EN），DMA 在忙时可搬数
 *
 * PowerControl(OFF) 会清掉 POWER|CFG|RX_EN，保留或随后 Uninitialize 清 INIT。
 * @{
 */
enum {
	FLG_INIT = (1U << 0),  /**< bit0：驱动已 Initialize */
	FLG_POWER = (1U << 1), /**< bit1：外设已上电 */
	FLG_CFG = (1U << 2),   /**< bit2：USART 异步模式已配置 */
	FLG_RX_EN = (1U << 3)  /**< bit3：软件允许 RX（非硬件 RE  alone） */
};
/** @} */

/** SWO.c 注册的事件回调（RECEIVE_COMPLETE / RX_OVERFLOW / FRAMING 等） */
static ARM_USART_SignalEvent_t s_cb_event;

/** 驱动状态标志，见上方 FLG_* */
static uint8_t s_flags;

/**
 * @brief 当前是否有一次 DMA 接收进行中
 * @note  1=忙：Receive 已启动且未 TC/Abort；GetStatus.rx_busy 由此导出
 */
static volatile uint8_t s_rx_busy;

/**
 * @brief 本次 Receive 请求的总字节数（DMA CNDTR 初值）
 */
static volatile uint32_t s_rx_num;

/**
 * @brief 已收到字节数快照
 * @note  TC 完成时 = s_rx_num；Abort/错误停 DMA 时用 CNDTR 计算后写入
 */
static volatile uint32_t s_rx_cnt;

/**
 * @brief 停止 DMA1 Ch5 与 USART1 RX DMA 请求，清除通道标志
 * @note  将 s_rx_busy 清 0；不修改 s_flags 中的 FLG_RX_EN
 */
static void swo_dma_stop(void) {
	DMA_Cmd(DMA1_Channel5, DISABLE);
	USART_DMACmd(USART1, USART_DMAReq_Rx, DISABLE);
	/* GL5=全局 TE5=传输错误 TC5=传输完成 HT5=半传输 */
	DMA_ClearFlag(DMA1_FLAG_GL5 | DMA1_FLAG_TC5 | DMA1_FLAG_HT5 | DMA1_FLAG_TE5);
	s_rx_busy = 0U;
}

/**
 * @brief 关闭 USART1 / DMA1 Ch5 / 相关 NVIC，并关 USART1 时钟
 * @note  由 PowerControl(OFF) 或 Uninitialize 在已上电时调用
 */
static void swo_hw_deinit(void) {
	NVIC_InitTypeDef NVIC_InitStructure;

	swo_dma_stop();

	USART_ITConfig(USART1, USART_IT_ERR, DISABLE);
	USART_Cmd(USART1, DISABLE);
	USART_DeInit(USART1);

	DMA_DeInit(DMA1_Channel5);

	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = SWO_USART_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_Init(&NVIC_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);
}

/**
 * @brief CMSIS：Initialize — 保存回调，置 FLG_INIT
 * @param cb_event 事件回调，可为 NULL（则 ISR 不上报）
 * @return ARM_DRIVER_OK
 */
static int32_t usart1_initialize(ARM_USART_SignalEvent_t cb_event) {
	s_cb_event = cb_event;
	s_flags = FLG_INIT;
	s_rx_busy = 0U;
	s_rx_num = 0U;
	s_rx_cnt = 0U;
	return ARM_DRIVER_OK;
}

/**
 * @brief CMSIS：Uninitialize — 若已上电则关硬件，清空全部状态
 * @return ARM_DRIVER_OK
 */
static int32_t usart1_uninitialize(void) {
	if ((s_flags & FLG_POWER) != 0U)
		swo_hw_deinit();

	s_cb_event = NULL;
	s_flags = 0U;
	s_rx_busy = 0U;
	s_rx_num = 0U;
	s_rx_cnt = 0U;
	return ARM_DRIVER_OK;
}

/**
 * @brief CMSIS：PowerControl — 上电/断电
 * @param state ARM_POWER_FULL：开 GPIOA/USART1/DMA1，PA10 浮空输入，使能 NVIC
 *              ARM_POWER_OFF ：调用 swo_hw_deinit，清除 POWER|CFG|RX_EN
 *              ARM_POWER_LOW ：不支持
 * @return ARM_DRIVER_OK / ERROR / ERROR_UNSUPPORTED
 *
 * @note FULL 成功后 s_flags |= FLG_POWER；须先有 FLG_INIT
 */
static int32_t usart1_power_control(ARM_POWER_STATE state) {
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	if ((s_flags & FLG_INIT) == 0U)
		return ARM_DRIVER_ERROR;

	switch (state) {
	case ARM_POWER_OFF:
		if ((s_flags & FLG_POWER) != 0U) {
			swo_hw_deinit();
			/* 保留 FLG_INIT，清掉上电后状态 */
			s_flags &= (uint8_t)~(FLG_POWER | FLG_CFG | FLG_RX_EN);
		}
		return ARM_DRIVER_OK;

	case ARM_POWER_FULL:
		if ((s_flags & FLG_POWER) != 0U)
			return ARM_DRIVER_OK;

		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
		RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

		/* PA10 = USART1_RX 浮空输入；不配置 PA9 */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &GPIO_InitStructure);

		DMA_DeInit(DMA1_Channel5);

		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = SWO_USART_IRQ_PRIORITY;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

		NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
		NVIC_Init(&NVIC_InitStructure);

		s_flags |= FLG_POWER;
		return ARM_DRIVER_OK;

	case ARM_POWER_LOW:
	default:
		return ARM_DRIVER_ERROR_UNSUPPORTED;
	}
}

/**
 * @brief CMSIS：Receive — 启动一次 DMA 单次接收
 * @param data 目标缓冲区（通常为 SWO TraceBuf 片段）
 * @param num  字节数，写入 DMA CNDTR
 * @return ARM_DRIVER_OK / ERROR_PARAMETER / ERROR / ERROR_BUSY
 *
 * @note 需要 FLG_POWER|FLG_CFG；若已 FLG_RX_EN 则立刻打开 USART_DMAReq_Rx
 *       完成后 DMA TC ISR 上报 ARM_USART_EVENT_RECEIVE_COMPLETE
 */
static int32_t usart1_receive(void *data, uint32_t num) {
	DMA_InitTypeDef DMA_InitStructure;

	if ((data == NULL) || (num == 0U))
		return ARM_DRIVER_ERROR_PARAMETER;
	if ((s_flags & (FLG_POWER | FLG_CFG)) != (FLG_POWER | FLG_CFG))
		return ARM_DRIVER_ERROR;
	if (s_rx_busy != 0U)
		return ARM_DRIVER_ERROR_BUSY;

	swo_dma_stop();

	s_rx_num = num;
	s_rx_cnt = 0U;
	s_rx_busy = 1U;

	/* USART1_RX 固定映射 DMA1 Channel5：外设→存储器、字节、Normal */
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)data;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = num;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);

	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC | DMA_IT_TE, ENABLE);
	DMA_ClearFlag(DMA1_FLAG_GL5 | DMA1_FLAG_TC5 | DMA1_FLAG_HT5 | DMA1_FLAG_TE5);
	DMA_Cmd(DMA1_Channel5, ENABLE);

	/* 仅当软件已允许 RX 时才开 DMA 请求，避免 Control(RX,0) 期间误收 */
	if ((s_flags & FLG_RX_EN) != 0U)
		USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);

	return ARM_DRIVER_OK;
}

/**
 * @brief CMSIS：GetRxCount — 本次 Receive 已收到字节数
 * @return 忙时用 (s_rx_num - CNDTR)；空闲时返回 Abort/TC 后写入的 s_rx_cnt
 */
static uint32_t usart1_get_rx_count(void) {
	uint32_t remaining;

	if (s_rx_busy == 0U)
		return s_rx_cnt;

	remaining = DMA_GetCurrDataCounter(DMA1_Channel5);
	if (remaining > s_rx_num)
		return s_rx_cnt;

	return (s_rx_num - remaining);
}

/**
 * @brief CMSIS：Control — 模式/波特率/RX 开关/中止接收
 * @param control 控制码（低 8 位模式 + 数据位/校验/停止位域）
 * @param arg     ASYNCHRONOUS 时为波特率；CONTROL_RX 时非 0=开、0=关
 * @return ARM_DRIVER_OK / ERROR / ERROR_PARAMETER / ERROR_UNSUPPORTED
 *
 * @par 支持的 control
 *   - ARM_USART_MODE_ASYNCHRONOUS | 8N1：配置 RX-only，置 FLG_CFG
 *   - ARM_USART_CONTROL_RX：置/清 FLG_RX_EN，并开关 DMAReq_Rx
 *   - ARM_USART_ABORT_RECEIVE：停 DMA，s_rx_cnt 记已收长度
 *   - ARM_USART_CONTROL_TX：空操作成功（SWO 无 TX）
 */
static int32_t usart1_control(uint32_t control, uint32_t arg) {
	USART_InitTypeDef USART_InitStructure;
	uint32_t mode;

	if ((s_flags & FLG_POWER) == 0U)
		return ARM_DRIVER_ERROR;

	mode = control & ARM_USART_CONTROL_Msk;

	switch (mode) {
	case ARM_USART_MODE_ASYNCHRONOUS:
		if (arg == 0U)
			return ARM_DRIVER_ERROR_PARAMETER;
		/* SWO UART 固定 8N1 */
		if ((control & ARM_USART_DATA_BITS_Msk) != ARM_USART_DATA_BITS_8)
			return ARM_DRIVER_ERROR_UNSUPPORTED;
		if ((control & ARM_USART_PARITY_Msk) != ARM_USART_PARITY_NONE)
			return ARM_DRIVER_ERROR_UNSUPPORTED;
		if ((control & ARM_USART_STOP_BITS_Msk) != ARM_USART_STOP_BITS_1)
			return ARM_DRIVER_ERROR_UNSUPPORTED;

		swo_dma_stop();

		USART_Cmd(USART1, DISABLE);
		USART_StructInit(&USART_InitStructure);
		USART_InitStructure.USART_BaudRate = arg;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Rx;
		USART_Init(USART1, &USART_InitStructure);

		/* ERR：ORE/FE/NE 经 USART1_IRQHandler 上报给 SWO */
		USART_ITConfig(USART1, USART_IT_ERR, ENABLE);
		USART_Cmd(USART1, ENABLE);

		s_flags |= FLG_CFG;
		return ARM_DRIVER_OK;

	case ARM_USART_CONTROL_RX:
		if (arg != 0U) {
			s_flags |= FLG_RX_EN;
			if (s_rx_busy != 0U)
				USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
			USART_Cmd(USART1, ENABLE);
		} else {
			s_flags &= (uint8_t)~FLG_RX_EN;
			USART_DMACmd(USART1, USART_DMAReq_Rx, DISABLE);
		}
		return ARM_DRIVER_OK;

	case ARM_USART_ABORT_RECEIVE:
		if (s_rx_busy != 0U) {
			s_rx_cnt = usart1_get_rx_count();
			swo_dma_stop();
		}
		return ARM_DRIVER_OK;

	case ARM_USART_CONTROL_TX:
		/* SWO 无发送，接受调用以免上层失败 */
		return ARM_DRIVER_OK;

	default:
		return ARM_DRIVER_ERROR_UNSUPPORTED;
	}
}

/**
 * @brief CMSIS：GetStatus — 目前仅填充 rx_busy
 */
static ARM_USART_STATUS usart1_get_status(void) {
	ARM_USART_STATUS status = {0};

	status.rx_busy = (s_rx_busy != 0U) ? 1U : 0U;
	return status;
}

/** @brief CMSIS：GetVersion */
static ARM_DRIVER_VERSION usart1_get_version(void) {
	ARM_DRIVER_VERSION version = {
		.api = ARM_USART_API_VERSION,
		.drv = ARM_DRIVER_VERSION_MAJOR_MINOR(1, 0),
	};
	return version;
}

/** @brief CMSIS：GetCapabilities — 仅声明异步 UART */
static ARM_USART_CAPABILITIES usart1_get_capabilities(void) {
	ARM_USART_CAPABILITIES caps = {0};

	caps.asynchronous = 1U;
	return caps;
}

/** @brief 不支持发送 */
static int32_t usart1_send(const void *data, uint32_t num) {
	(void)data;
	(void)num;
	return ARM_DRIVER_ERROR_UNSUPPORTED;
}

/** @brief 不支持全双工 Transfer */
static int32_t usart1_transfer(const void *data_out, void *data_in, uint32_t num) {
	(void)data_out;
	(void)data_in;
	(void)num;
	return ARM_DRIVER_ERROR_UNSUPPORTED;
}

/** @brief 无 TX，恒为 0 */
static uint32_t usart1_get_tx_count(void) {
	return 0U;
}

/** @brief 不支持 Modem 线 */
static int32_t usart1_set_modem_control(ARM_USART_MODEM_CONTROL control) {
	(void)control;
	return ARM_DRIVER_ERROR_UNSUPPORTED;
}

/** @brief 不支持 Modem 状态 */
static ARM_USART_MODEM_STATUS usart1_get_modem_status(void) {
	ARM_USART_MODEM_STATUS status = {0};
	return status;
}

/**
 * @brief CMSIS-Driver 实例 Driver_USART1
 * @note  DAP_config.h 中 SWO_UART_DRIVER=1，SWO.c 链接本符号
 */
ARM_DRIVER_USART Driver_USART1 = {
	.GetVersion = usart1_get_version,
	.GetCapabilities = usart1_get_capabilities,
	.Initialize = usart1_initialize,
	.Uninitialize = usart1_uninitialize,
	.PowerControl = usart1_power_control,
	.Send = usart1_send,
	.Receive = usart1_receive,
	.Transfer = usart1_transfer,
	.GetTxCount = usart1_get_tx_count,
	.GetRxCount = usart1_get_rx_count,
	.Control = usart1_control,
	.GetStatus = usart1_get_status,
	.SetModemControl = usart1_set_modem_control,
	.GetModemStatus = usart1_get_modem_status,
};

/**
 * @brief DMA1 Channel5 中断（USART1_RX）
 *
 * @par 事件标志（传给 s_cb_event，供 SWO.c 使用）
 * | 标志 | 含义 |
 * |------|------|
 * | ARM_USART_EVENT_RECEIVE_COMPLETE | CNDTR 收满，一块 Trace 完成 |
 * | ARM_USART_EVENT_RX_OVERFLOW      | DMA TE，按溢出上报 |
 *
 * @note 禁止调用 FreeRTOS API
 */
void DMA1_Channel5_IRQHandler(void) {
	uint32_t event = 0U;

	if (DMA_GetITStatus(DMA1_IT_TE5) == SET) {
		DMA_ClearITPendingBit(DMA1_IT_TE5);
		s_rx_cnt = usart1_get_rx_count();
		swo_dma_stop();
		event |= ARM_USART_EVENT_RX_OVERFLOW;
	}

	if (DMA_GetITStatus(DMA1_IT_TC5) == SET) {
		DMA_ClearITPendingBit(DMA1_IT_TC5);
		s_rx_cnt = s_rx_num;
		USART_DMACmd(USART1, USART_DMAReq_Rx, DISABLE);
		DMA_Cmd(DMA1_Channel5, DISABLE);
		s_rx_busy = 0U;
		event |= ARM_USART_EVENT_RECEIVE_COMPLETE;
	}

	if ((event != 0U) && (s_cb_event != NULL))
		s_cb_event(event);
}

/**
 * @brief USART1 错误中断（USART_IT_ERR）
 *
 * @par USART SR 硬件标志 → CMSIS 事件
 * | SR 位 | 含义           | 上报事件 |
 * |-------|----------------|----------|
 * | ORE   | 溢出（未及时读）| ARM_USART_EVENT_RX_OVERFLOW |
 * | FE/NE | 帧错误/噪声    | ARM_USART_EVENT_RX_FRAMING_ERROR |
 * | PE    | 校验错误        | ARM_USART_EVENT_RX_PARITY_ERROR |
 *
 * @note 读 DR 以清除错误标志；SWO 为 8N1，PE 理论上不应出现
 * @note 禁止调用 FreeRTOS API
 */
void USART1_IRQHandler(void) {
	uint32_t event = 0U;
	uint32_t sr;

	sr = USART1->SR;

	if ((sr & (USART_FLAG_ORE | USART_FLAG_FE | USART_FLAG_NE | USART_FLAG_PE)) != 0U) {
		/* 读 DR 清除 ORE/FE/NE/PE */
		(void)USART1->DR;

		if ((sr & USART_FLAG_ORE) != 0U)
			event |= ARM_USART_EVENT_RX_OVERFLOW;
		if ((sr & (USART_FLAG_FE | USART_FLAG_NE)) != 0U)
			event |= ARM_USART_EVENT_RX_FRAMING_ERROR;
		if ((sr & USART_FLAG_PE) != 0U)
			event |= ARM_USART_EVENT_RX_PARITY_ERROR;
	}

	if ((event != 0U) && (s_cb_event != NULL))
		s_cb_event(event);
}

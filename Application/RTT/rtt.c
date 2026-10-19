/**
 * @file    rtt.c
 * @brief   SEGGER RTT 兼容实现，供 CMSIS-DAP + OpenOCD 实时打印
 *
 * printf/_write 同时输出到 RTT 与 SWO（ITM），便于两种通道联调。
 */
#include "rtt.h"
#include "swo.h"

#include "stm32f10x.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int _write(int fd, char *pBuffer, int size);

#define RTT_MAX_UP_BUFFERS   1
#define RTT_MAX_DOWN_BUFFERS 1
#define RTT_UP_BUF_SIZE      1024U
#define RTT_DOWN_BUF_SIZE    16U
#define RTT_PRINTF_BUF_SIZE  128U

#define RTT_MODE_SKIP 0U

typedef struct {
	const char *sName;
	char *pBuffer;
	unsigned SizeOfBuffer;
	unsigned WrOff;
	volatile unsigned RdOff;
	unsigned Flags;
} SeggerRttBufferUp;

typedef struct {
	const char *sName;
	char *pBuffer;
	unsigned SizeOfBuffer;
	volatile unsigned WrOff;
	unsigned RdOff;
	unsigned Flags;
} SeggerRttBufferDown;

typedef struct {
	char acID[16];
	int MaxNumUpBuffers;
	int MaxNumDownBuffers;
	SeggerRttBufferUp aUp[RTT_MAX_UP_BUFFERS];
	SeggerRttBufferDown aDown[RTT_MAX_DOWN_BUFFERS];
} SeggerRttCb;

static char s_up_buf[RTT_UP_BUF_SIZE];
static char s_down_buf[RTT_DOWN_BUF_SIZE];

/* 符号名必须为 _SEGGER_RTT，cortex-debug / OpenOCD 按此查找 */
SeggerRttCb _SEGGER_RTT __attribute__((used, aligned(4))) = {
	.acID = {'S', 'E', 'G', 'G', 'E', 'R', ' ', 'R', 'T', 'T', '\0'},
	.MaxNumUpBuffers = RTT_MAX_UP_BUFFERS,
	.MaxNumDownBuffers = RTT_MAX_DOWN_BUFFERS,
	.aUp = {
		{
			.sName = "Terminal",
			.pBuffer = s_up_buf,
			.SizeOfBuffer = RTT_UP_BUF_SIZE,
			.WrOff = 0U,
			.RdOff = 0U,
			.Flags = RTT_MODE_SKIP,
		},
	},
	.aDown = {
		{
			.sName = "Terminal",
			.pBuffer = s_down_buf,
			.SizeOfBuffer = RTT_DOWN_BUF_SIZE,
			.WrOff = 0U,
			.RdOff = 0U,
			.Flags = RTT_MODE_SKIP,
		},
	},
};

static uint32_t rtt_lock(void)
{
	uint32_t primask = __get_PRIMASK();

	__disable_irq();
	return primask;
}

static void rtt_unlock(uint32_t primask)
{
	if (primask == 0U)
		__enable_irq();
}

void rtt_init(void)
{
	/* .data 已带控制块初值；此处再写一遍 ID，保证调试器可搜索 */
	_SEGGER_RTT.acID[0] = 'S';
	_SEGGER_RTT.acID[1] = 'E';
	_SEGGER_RTT.acID[2] = 'G';
	_SEGGER_RTT.acID[3] = 'G';
	_SEGGER_RTT.acID[4] = 'E';
	_SEGGER_RTT.acID[5] = 'R';
	_SEGGER_RTT.acID[6] = ' ';
	_SEGGER_RTT.acID[7] = 'R';
	_SEGGER_RTT.acID[8] = 'T';
	_SEGGER_RTT.acID[9] = 'T';
	_SEGGER_RTT.acID[10] = '\0';
}

uint32_t rtt_write(uint32_t buf_index, const void *pBuffer, uint32_t num_bytes)
{
	const uint8_t *pSrc = (const uint8_t *)pBuffer;
	SeggerRttBufferUp *pRing;
	unsigned rd_off;
	unsigned wr_off;
	unsigned avail;
	unsigned remaining;
	unsigned chunk;
	uint32_t primask;

	if ((pBuffer == NULL) || (num_bytes == 0U) || (buf_index >= RTT_MAX_UP_BUFFERS))
		return 0U;

	pRing = &_SEGGER_RTT.aUp[buf_index];
	primask = rtt_lock();

	rd_off = pRing->RdOff;
	wr_off = pRing->WrOff;
	if (rd_off <= wr_off)
		avail = pRing->SizeOfBuffer - 1U - wr_off + rd_off;
	else
		avail = rd_off - wr_off - 1U;

	/* SKIP：空间不足则丢弃整包，避免半截日志 */
	if (num_bytes > avail) {
		rtt_unlock(primask);
		return 0U;
	}

	remaining = num_bytes;
	while (remaining != 0U) {
		unsigned space_to_end = pRing->SizeOfBuffer - wr_off;

		chunk = remaining;
		if (chunk > space_to_end)
			chunk = space_to_end;
		memcpy(&pRing->pBuffer[wr_off], pSrc, chunk);
		pSrc += chunk;
		remaining -= chunk;
		wr_off += chunk;
		if (wr_off >= pRing->SizeOfBuffer)
			wr_off = 0U;
	}

	__DMB();
	pRing->WrOff = wr_off;
	rtt_unlock(primask);
	return num_bytes;
}

int rtt_printf(const char *pFmt, ...)
{
	char buf[RTT_PRINTF_BUF_SIZE];
	va_list args;
	int len;

	if (pFmt == NULL)
		return -1;

	va_start(args, pFmt);
	len = vsnprintf(buf, sizeof(buf), pFmt, args);
	va_end(args);

	if (len <= 0)
		return len;
	if (len >= (int)sizeof(buf))
		len = (int)sizeof(buf) - 1;

	return (int)rtt_write(0U, buf, (uint32_t)len);
}



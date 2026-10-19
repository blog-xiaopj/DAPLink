#ifndef SWO_H
#define SWO_H

#include <stdint.h>

/**
 * @brief 初始化 ITM/TPIU SWO（STM32F103：PB3 = TRACESWO，UART NRZ）
 * @param cpu_hz  CPU 时钟 Hz（通常 72000000）
 * @param swo_hz  SWO 波特率 Hz（须与 DAP 端一致，建议 2000000）
 * @return 0=成功，非0=参数错误
 *
 * @note 会关闭 JTAG、保留 SWD，以释放 PB3。
 *       将目标 PB3 接到 DAPLink PA10（USART1_RX）即可抓包。
 */
uint8_t swo_init(uint32_t cpu_hz, uint32_t swo_hz);

/**
 * @brief 经 ITM port0 发送一字节（调试器未接时可能阻塞或丢弃）
 * @param ch 字符
 */
void swo_putchar(uint8_t ch);

/**
 * @brief 经 ITM port0 发送缓冲区
 * @param pBuffer 数据
 * @param size    字节数
 * @return 实际发送字节数
 */
uint32_t swo_write(const void *pBuffer, uint32_t size);

#endif /* SWO_H */

#ifndef UART2_H
#define UART2_H

#include <stdint.h>

/**
 * @brief 初始化 USART2（PA2 TX / PA3 RX，8N1，RX 中断）
 * @param baudrate 波特率
 * @return uint8_t 0=成功, 1=参数错误
 */
uint8_t uart2_init(uint32_t baudrate);

/**
 * @brief 通过 USART2 阻塞发送
 * @param pBuffer 数据指针
 * @param size    字节数
 */
void uart2_write(const uint8_t *pBuffer, uint32_t size);

/**
 * @brief 从接收环形缓冲取出数据（任务上下文）
 * @param pBuffer 输出缓冲
 * @param size    最大字节数
 * @return 实际读出字节数
 */
uint32_t uart2_read(uint8_t *pBuffer, uint32_t size);

#endif /* UART2_H */

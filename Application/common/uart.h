#ifndef UART_H
#define UART_H

#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "stream_buffer.h"

typedef struct {
	uint32_t baudrate; /**< 波特率 */
	uint8_t data_bits; /**< 数据位（8 或 9） */
	uint8_t stop_bits; /**< 停止位（0=1 stop，1=1.5 stop，2=2 stop） */
	uint8_t parity;    /**< 奇偶校验（0=无，1=奇，2=偶） */
} uart_config_t;

/**
 * @brief 初始化 USART2（PA2 TX / PA3 RX，默认 115200 8N1）
 */
void uart2_init(void);

/**
 * @brief 按 CDC 线路编码配置 USART2
 * @param config 线路编码
 */
void uart2_config_set(const uart_config_t *config);

/**
 * @brief 读取 USART2 当前线路编码
 * @param config 输出
 */
void uart2_config_get(uart_config_t *config);

/**
 * @brief 向 USART2 发送
 * @param pBuffer 数据指针
 * @param size    字节数
 */
void uart2_write(const uint8_t *pBuffer, size_t size);

/**
 * @brief USART2 收到数据（弱符号，应用层可重写）
 * @param pBuffer 数据指针
 * @param size    字节数
 */
void uart2_read(const uint8_t *pBuffer, size_t size);

/**
 * @brief USART2 收发方向（弱符号，可在 ISR 中调用）
 * @param is_tx 1=发送（RS485 DE 有效），0=接收
 */
void uart2_dir_set(uint8_t is_tx);

/**
 * @brief 初始化 USART3（PB10 TX / PB11 RX，默认 115200 8N1）
 */
void uart3_init(void);

/**
 * @brief 按 CDC 线路编码配置 USART3
 * @param config 线路编码
 */
void uart3_config_set(const uart_config_t *config);

/**
 * @brief 读取 USART3 当前线路编码
 * @param config 输出
 */
void uart3_config_get(uart_config_t *config);

/**
 * @brief 向 USART3 发送
 * @param pBuffer 数据指针
 * @param size    字节数
 */
void uart3_write(const uint8_t *pBuffer, size_t size);

/**
 * @brief USART3 收到数据（弱符号，应用层可重写）
 * @param pBuffer 数据指针
 * @param size    字节数
 */
void uart3_read(const uint8_t *pBuffer, size_t size);

/**
 * @brief USART3 收发方向（弱符号，可在 ISR 中调用）
 * @param is_tx 1=发送（RS485 DE 有效），0=接收
 */
void uart3_dir_set(uint8_t is_tx);

#endif

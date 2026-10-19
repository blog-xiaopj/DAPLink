#ifndef RTT_H
#define RTT_H

#include <stdint.h>

/**
 * @brief 初始化 SEGGER RTT 控制块（CMSIS-DAP / OpenOCD 可扫描）
 */
void rtt_init(void);

/**
 * @brief 向 RTT 上行通道写入数据
 * @param buf_index 通道号，终端为 0
 * @param pBuffer   数据指针
 * @param num_bytes 字节数
 * @return 实际写入字节数
 */
uint32_t rtt_write(uint32_t buf_index, const void *pBuffer, uint32_t num_bytes);

/**
 * @brief 通过 RTT 通道 0 格式化输出
 * @param pFmt 格式化字符串
 * @return 写入字节数，失败返回负值
 */
int rtt_printf(const char *pFmt, ...);

#endif /* RTT_H */

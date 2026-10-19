#ifndef __USB_DAP_H__
#define __USB_DAP_H__

#include <stddef.h>
#include <stdint.h>

/**
 * @brief 初始化 CMSIS-DAP v1（HID）+ 两路 CDC ACM 复合设备
 */
void usb_dap_init(void);

/**
 * @brief 第 1 路 USB 虚拟串口收到数据（弱符号，应用层可重写）
 * @param pBuffer 数据指针
 * @param size    字节数
 */
void usbd_cdc_acm_received(const char *pBuffer, size_t size);

/**
 * @brief 向第 1 路 USB 虚拟串口发送数据（任务上下文）
 * @param pBuffer 数据指针
 * @param size    字节数
 */
void usbd_cdc_acm_transfer(const char *pBuffer, size_t size);

/**
 * @brief 第 2 路 USB 虚拟串口收到数据（弱符号，应用层可重写）
 * @param pBuffer 数据指针
 * @param size    字节数
 */
void usbd_cdc_acm2_received(const char *pBuffer, size_t size);

/**
 * @brief 向第 2 路 USB 虚拟串口发送数据（任务上下文）
 * @param pBuffer 数据指针
 * @param size    字节数
 */
void usbd_cdc_acm2_transfer(const char *pBuffer, size_t size);

#endif

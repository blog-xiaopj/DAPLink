#ifndef __USB_DAP_H__
#define __USB_DAP_H__

#include <stddef.h>
#include <stdint.h>

/**
 * @brief WinUSB / Microsoft OS 描述符版本
 * - 1: MS OS 1.0（字符串 0xEE / WCID，Windows 8/7 兼容）
 * - 2: MS OS 2.0（BOS，Windows 8.1+，推荐）
 * @note 可用编译选项 -DCONFIG_WINUSB_VERSION=1 覆盖默认值
 */
#ifndef CONFIG_WINUSB_VERSION
#define CONFIG_WINUSB_VERSION 2
#endif

#if (CONFIG_WINUSB_VERSION != 1) && (CONFIG_WINUSB_VERSION != 2)
#error "CONFIG_WINUSB_VERSION must be 1 (MS OS 1.0) or 2 (MS OS 2.0)"
#endif

/**
 * @brief 初始化 CMSIS-DAP v2（WinUSB Bulk）+ 两路 CDC ACM 复合设备
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

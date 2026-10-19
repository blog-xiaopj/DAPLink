/**
 * @file    usb_config.h
 * @brief   CherryUSB 设备栈配置（STM32F103 USB FS CDC ACM）
 */
 #ifndef CHERRYUSB_CONFIG_H
 #define CHERRYUSB_CONFIG_H
 
//  #define CONFIG_USB_PRINTF(...) ((void)0)
//  #define CONFIG_USB_DBG_LEVEL   USB_DBG_ERROR
 #define CONFIG_USB_PRINTF(...) printf(__VA_ARGS__)
 #define CONFIG_USB_DBG_LEVEL   USB_DBG_INFO
 #define CONFIG_USB_ASSERT_DISABLE
 
 #define CONFIG_USB_ALIGN_SIZE 4
 #define USB_NOCACHE_RAM_SECTION
 
 /* 须大于配置描述符长度：9 + CDC_ACM_DESCRIPTOR_LEN * 端口数（2 路约 141 字节） */
 #define CONFIG_USBDEV_REQUEST_BUFFER_LEN 256
 #define CONFIG_USBDEV_ADVANCE_DESC
 #define CONFIG_USBDEV_MAX_BUS            1
 
 /* STM32F103 PMA 为 16 位交错访问 */
 #define CONFIG_USBDEV_FSDEV_PMA_ACCESS 2
 #define CONFIG_USB_FSDEV_RAM_SIZE      512
 
 #ifndef USB_BASE
 #define USB_BASE 0x40005C00UL
 #endif
 
 #endif /* CHERRYUSB_CONFIG_H */
 
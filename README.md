## 简介

- 基于**STM32F103C8**实现的DAPLink调试器, 支持下载、仿真、打印。
- 支持HID和WinUSB两种协议。
- 支持SWO打印和RTT打印。
- 支持JTAG和SWD两种调试方式。
- 支持两路CDC串口，搭配底板可使用232/485进行通信，并引出TTL信号。
- 上电自动枚举USB2.0,无单独引脚用于枚举。


- 核心板: 闲置的[立创·地阔星STM32F103C8T6开发板](https://lckfb.com/project/detail/lckfb-dkx-stm32f103c8t6)
- 底板: [LCEDA设计]( https://oshwhub.com/monkey2020/project_erinywct)_仅作为232/485功能扩展及type-A接口扩展，非必须_ 
- 源码及镜像: [https://github.com/blog-xiaopj/DAPLink](https://github.com/blog-xiaopj/DAPLink)
- 详细设计文档: [https://xiaopj.com/p/daplink/](https://xiaopj.com/p/daplink/)


### 调试口

| 功能 | 引脚 |
|------|------|
| SWDIO / TMS | PA4 |
| SWCLK / TCK | PA5 |
| JTDO | PA6 |
| TDI | PA7 |
| SWO（USART1_RX） | PA10 |
| nTRST | PA1（开漏，低有效） |
| nRESET | PB9（开漏，低有效） |
| LED | PC13 |

底板上短接 **PA6（JTDO）** 与 **PA10（SWO）**，再接到调试座 TDO/SWO（ST-Link 式）。两脚必须保持输入，禁止配成输出。

### 虚拟串口（探针）

| CDC | 外设 | 引脚 |
|-----|------|------|
| UART1 RS232 | USART2 | PA2 TX / PA3 RX |
| UART2 RS485 | USART3 | PB10 TX / PB11 RX |

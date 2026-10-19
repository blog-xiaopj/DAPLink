/*
 * Copyright (c) 2013-2021 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        16. June 2021
 * $Revision:    V2.1.0
 *
 * Project:      CMSIS-DAP Configuration
 * Title:        DAP_config.h CMSIS-DAP Configuration File (Template)
 *
 *---------------------------------------------------------------------------*/

#ifndef __DAP_CONFIG_H__
#define __DAP_CONFIG_H__

#include <stdint.h>
#include <stddef.h>
#include "cmsis_gcc.h"

//**************************************************************************************************
/**
\defgroup DAP_Config_Debug_gr CMSIS-DAP Debug Unit Information
\ingroup DAP_ConfigIO_gr
@{
Provides definitions about the hardware and configuration of the Debug Unit.

This information includes:
 - Definition of Cortex-M processor parameters used in CMSIS-DAP Debug Unit.
 - Debug Unit Identification strings (Vendor, Product, Serial Number).
 - Debug Unit communication packet size.
 - Debug Access Port supported modes and settings (JTAG/SWD and SWO).
 - Optional information about a connected Target Device (for Evaluation Boards).
*/

#ifdef _RTE_
#include "RTE_Components.h"
#include CMSIS_device_header
#else
// #include "device.h"                             // Debug Unit Cortex-M Processor Header File
#include "stm32f10x.h"
#endif

/// Processor Clock of the Cortex-M MCU used in the Debug Unit.
/// This value is used to calculate the SWD/JTAG clock speed.
/// Must be a compile-time constant (used in #if and clock math).
#define CPU_CLOCK               72000000U       ///< Specifies the CPU Clock in Hz.

/// Number of processor cycles for I/O Port write operations.
/// This value is used to calculate the SWD/JTAG clock speed that is generated with I/O
/// Port write operations in the Debug Unit by a Cortex-M MCU. Most Cortex-M processors
/// require 2 processor cycles for a I/O Port Write operation.  If the Debug Unit uses
/// a Cortex-M0+ processor with high-speed peripheral I/O only 1 processor cycle might be
/// required.
#define IO_PORT_WRITE_CYCLES    2U              ///< I/O Cycles: 2=default, 1=Cortex-M0+ fast I/0.

/// Indicate that Serial Wire Debug (SWD) communication mode is available at the Debug Access Port.
/// This information is returned by the command \ref DAP_Info as part of <b>Capabilities</b>.
#define DAP_SWD                 1               ///< SWD Mode:  1 = available, 0 = not available.

/// Indicate that JTAG communication mode is available at the Debug Port.
/// This information is returned by the command \ref DAP_Info as part of <b>Capabilities</b>.
#define DAP_JTAG                1               ///< JTAG Mode: 1 = available, 0 = not available.

/// Configure maximum number of JTAG devices on the scan chain connected to the Debug Access Port.
/// This setting impacts the RAM requirements of the Debug Unit. Valid range is 1 .. 255.
#define DAP_JTAG_DEV_CNT        8U              ///< Maximum number of JTAG devices on scan chain.

/// Default communication mode on the Debug Access Port.
/// Used for the command \ref DAP_Connect when Port Default mode is selected.
#define DAP_DEFAULT_PORT        1U              ///< Default JTAG/SWJ Port Mode: 1 = SWD, 2 = JTAG.

/// Default communication speed on the Debug Access Port for SWD and JTAG mode.
/// Used to initialize the default SWD/JTAG clock frequency.
/// The command \ref DAP_SWJ_Clock can be used to overwrite this default setting.
#define DAP_DEFAULT_SWJ_CLOCK   1000000U        ///< Default SWD/JTAG clock frequency in Hz.

/// Maximum Package Size for Command and Response data.
/// This configuration settings is used to optimize the communication performance with the
/// debugger and depends on the USB peripheral. Typical vales are 64 for Full-speed USB HID or WinUSB,
/// 1024 for High-speed USB HID and 512 for High-speed USB WinUSB.
// #define DAP_PACKET_SIZE         512U            ///< Specifies Packet Size in bytes.
#define DAP_PACKET_SIZE         64U            ///< Specifies Packet Size in bytes.

/// Maximum Package Buffers for Command and Response data.
/// This configuration settings is used to optimize the communication performance with the
/// debugger and depends on the USB peripheral. For devices with limited RAM or USB buffer the
/// setting can be reduced (valid range is 1 .. 255).
// #define DAP_PACKET_COUNT        8U              ///< Specifies number of packets buffered.
#define DAP_PACKET_COUNT        4U       ///< Buffers: 4 for STM32F103 20KB RAM.

/// Indicate that UART Serial Wire Output (SWO) trace is available.
/// This information is returned by the command \ref DAP_Info as part of <b>Capabilities</b>.
#define SWO_UART                1               ///< SWO UART:  1 = available, 0 = not available.

/// USART Driver instance number for the UART SWO.
#define SWO_UART_DRIVER         1               ///< USART Driver instance number (Driver_USART#).

/// Maximum SWO UART Baudrate.
#define SWO_UART_MAX_BAUDRATE   10000000U        ///< SWO UART Maximum Baudrate in Hz.

/// Indicate that Manchester Serial Wire Output (SWO) trace is available.
/// This information is returned by the command \ref DAP_Info as part of <b>Capabilities</b>.
#define SWO_MANCHESTER          0               ///< SWO Manchester:  1 = available, 0 = not available.

/// SWO Trace Buffer Size.
#define SWO_BUFFER_SIZE         256U            ///< SWO Trace Buffer Size in bytes (must be 2^n).

/// SWO Streaming Trace.
#define SWO_STREAM              0               ///< SWO Streaming Trace: 1 = available, 0 = not available.

/// Clock frequency of the Test Domain Timer. Timer value is returned with \ref TIMESTAMP_GET.
#define TIMESTAMP_CLOCK         72000000U       ///< Timestamp clock in Hz (0 = timestamps not supported).

/// Indicate that UART Communication Port is available.
/// This information is returned by the command \ref DAP_Info as part of <b>Capabilities</b>.
#define DAP_UART                0               ///< DAP UART:  1 = available, 0 = not available.

/// USART Driver instance number for the UART Communication Port.
#define DAP_UART_DRIVER         0               ///< USART Driver instance number (Driver_USART#).

/// UART Receive Buffer Size.
#define DAP_UART_RX_BUFFER_SIZE 1024U           ///< Uart Receive Buffer Size in bytes (must be 2^n).

/// UART Transmit Buffer Size.
#define DAP_UART_TX_BUFFER_SIZE 1024U           ///< Uart Transmit Buffer Size in bytes (must be 2^n).

/// Indicate that UART Communication via USB COM Port is available.
/// This information is returned by the command \ref DAP_Info as part of <b>Capabilities</b>.
#define DAP_UART_USB_COM_PORT   0               ///< USB COM Port:  1 = available, 0 = not available.

/// Debug Unit is connected to fixed Target Device.
/// The Debug Unit may be part of an evaluation board and always connected to a fixed
/// known device. In this case a Device Vendor, Device Name, Board Vendor and Board Name strings
/// are stored and may be used by the debugger or IDE to configure device parameters.
#define TARGET_FIXED            0               ///< Target: 1 = known, 0 = unknown;

#define TARGET_DEVICE_VENDOR    "Arm"           ///< String indicating the Silicon Vendor
#define TARGET_DEVICE_NAME      "Cortex-M"      ///< String indicating the Target Device
#define TARGET_BOARD_VENDOR     "Arm"           ///< String indicating the Board Vendor
#define TARGET_BOARD_NAME       "Arm board"     ///< String indicating the Board Name

#if TARGET_FIXED != 0
#include <string.h>
static const char TargetDeviceVendor [] = TARGET_DEVICE_VENDOR;
static const char TargetDeviceName   [] = TARGET_DEVICE_NAME;
static const char TargetBoardVendor  [] = TARGET_BOARD_VENDOR;
static const char TargetBoardName    [] = TARGET_BOARD_NAME;
#endif

/** Get Vendor Name string.
\param str Pointer to buffer to store the string (max 60 characters).
\return String length (including terminating NULL character) or 0 (no string).
*/
__STATIC_INLINE uint8_t DAP_GetVendorString (char *str) {
  (void)str;
  return (0U);
}

/** Get Product Name string.
\param str Pointer to buffer to store the string (max 60 characters).
\return String length (including terminating NULL character) or 0 (no string).
*/
__STATIC_INLINE uint8_t DAP_GetProductString (char *str) {
  (void)str;
  return (0U);
}

/** Get Serial Number string.
\param str Pointer to buffer to store the string (max 60 characters).
\return String length (including terminating NULL character) or 0 (no string).
*/
__STATIC_INLINE uint8_t DAP_GetSerNumString (char *str) {
  (void)str;
  return (0U);
}

/** Get Target Device Vendor string.
\param str Pointer to buffer to store the string (max 60 characters).
\return String length (including terminating NULL character) or 0 (no string).
*/
__STATIC_INLINE uint8_t DAP_GetTargetDeviceVendorString (char *str) {
#if TARGET_FIXED != 0
  uint8_t len;

  strcpy(str, TargetDeviceVendor);
  len = (uint8_t)(strlen(TargetDeviceVendor) + 1U);
  return (len);
#else
  (void)str;
  return (0U);
#endif
}

/** Get Target Device Name string.
\param str Pointer to buffer to store the string (max 60 characters).
\return String length (including terminating NULL character) or 0 (no string).
*/
__STATIC_INLINE uint8_t DAP_GetTargetDeviceNameString (char *str) {
#if TARGET_FIXED != 0
  uint8_t len;

  strcpy(str, TargetDeviceName);
  len = (uint8_t)(strlen(TargetDeviceName) + 1U);
  return (len);
#else
  (void)str;
  return (0U);
#endif
}

/** Get Target Board Vendor string.
\param str Pointer to buffer to store the string (max 60 characters).
\return String length (including terminating NULL character) or 0 (no string).
*/
__STATIC_INLINE uint8_t DAP_GetTargetBoardVendorString (char *str) {
#if TARGET_FIXED != 0
  uint8_t len;

  strcpy(str, TargetBoardVendor);
  len = (uint8_t)(strlen(TargetBoardVendor) + 1U);
  return (len);
#else
  (void)str;
  return (0U);
#endif
}

/** Get Target Board Name string.
\param str Pointer to buffer to store the string (max 60 characters).
\return String length (including terminating NULL character) or 0 (no string).
*/
__STATIC_INLINE uint8_t DAP_GetTargetBoardNameString (char *str) {
#if TARGET_FIXED != 0
  uint8_t len;

  strcpy(str, TargetBoardName);
  len = (uint8_t)(strlen(TargetBoardName) + 1U);
  return (len);
#else
  (void)str;
  return (0U);
#endif
}

/** Get Product Firmware Version string.
\param str Pointer to buffer to store the string (max 60 characters).
\return String length (including terminating NULL character) or 0 (no string).
*/
__STATIC_INLINE uint8_t DAP_GetProductFirmwareVersionString (char *str) {
  (void)str;
  return (0U);
}

///@}


//**************************************************************************************************
/**
\defgroup DAP_Config_PortIO_gr CMSIS-DAP Hardware I/O Pin Access
\ingroup DAP_ConfigIO_gr
@{

Standard I/O Pins of the CMSIS-DAP Hardware Debug Port support standard JTAG mode
and Serial Wire Debug (SWD) mode. In SWD mode only 2 pins are required to implement the debug
interface of a device. The following I/O Pins are provided:

JTAG I/O Pin                 | SWD I/O Pin          | CMSIS-DAP Hardware pin mode
---------------------------- | -------------------- | ---------------------------------------------
TCK: Test Clock              | SWCLK: Clock         | Output Push/Pull
TMS: Test Mode Select        | SWDIO: Data I/O      | Output Push/Pull; Input (for receiving data)
TDI: Test Data Input         |                      | Output Push/Pull
TDO: Test Data Output        |                      | Input
nTRST: Test Reset (optional) |                      | Output Open Drain with pull-up resistor
nRESET: Device Reset         | nRESET: Device Reset | Output Open Drain with pull-up resistor


DAP Hardware I/O Pin Access Functions
-------------------------------------
The various I/O Pins are accessed by functions that implement the Read, Write, Set, or Clear to
these I/O Pins.

For the SWDIO I/O Pin there are additional functions that are called in SWD I/O mode only.
This functions are provided to achieve faster I/O that is possible with some advanced GPIO
peripherals that can independently write/read a single I/O pin without affecting any other pins
of the same I/O port. The following SWDIO I/O Pin functions are provided:
 - \ref PIN_SWDIO_OUT_ENABLE to enable the output mode from the DAP hardware.
 - \ref PIN_SWDIO_OUT_DISABLE to enable the input mode to the DAP hardware.
 - \ref PIN_SWDIO_IN to read from the SWDIO I/O pin with utmost possible speed.
 - \ref PIN_SWDIO_OUT to write to the SWDIO I/O pin with utmost possible speed.
*/

/*
 * STM32F103C8 debug I/O
 *   SWDIO/TMS  PA4     SWCLK/TCK  PA5
 *   TDI        PA7     JTDO       PA6
 *   nRESET     PB9     nTRST      PA1  (open-drain; 0 = assert)
 *   LED        PC13    (active high)
 *   SWO UART   PA10    (USART1_RX, DMA1 Ch5)
 *
 * ST-Link 式接法：PCB 上短接 PA6(JTDO) 与 PA10(SWO)，再接到调试座 TDO/SWO。
 * 两脚必须始终为输入（浮空），禁止配成输出，以免对顶。
 *
 * This file follows CMSIS-DAP/x893 open-drain polarity (0 = reset).
 * SWDIO 若在 pin0-7，PIN_SWDIO_OUT_ENABLE/DISABLE 必须改 CRL（勿写死 CRH）。
 */

#define PIN_SWDIO_TMS_PORT      GPIOA
#define PIN_SWDIO_TMS_BIT       4U
#define PIN_SWCLK_TCK_PORT      GPIOA
#define PIN_SWCLK_TCK_BIT       5U
#define PIN_TDO_PORT            GPIOA
#define PIN_TDO_BIT             6U
#define PIN_TDI_PORT            GPIOA
#define PIN_TDI_BIT             7U
#define PIN_nTRST_PORT          GPIOA
#define PIN_nTRST_BIT           1U
#define PIN_nRESET_PORT         GPIOB
#define PIN_nRESET_BIT          9U
#define PIN_LED_PORT            GPIOC
#define PIN_LED_BIT             13U

#define PIN_SWCLK_TCK_MASK      (1U << PIN_SWCLK_TCK_BIT)
#define PIN_SWDIO_TMS_MASK      (1U << PIN_SWDIO_TMS_BIT)
#define PIN_TDI_MASK            (1U << PIN_TDI_BIT)
#define PIN_TDO_MASK            (1U << PIN_TDO_BIT)
#define PIN_nTRST_MASK          (1U << PIN_nTRST_BIT)
#define PIN_nRESET_MASK         (1U << PIN_nRESET_BIT)
#define PIN_LED_MASK            (1U << PIN_LED_BIT)

/* STM32F1 CNF+MODE nibble: CNF[1:0] MODE[1:0] */
#define DAP_GPIO_IN_FLOAT       0x4U    /* CNF=01 MODE=00 浮空输入 */
#define DAP_GPIO_IN_PU          0x8U    /* CNF=10 MODE=00, ODR=1 上拉输入 */
#define DAP_GPIO_OUT_PP_50      0x3U    /* CNF=00 MODE=11 推挽输出50MHz */
#define DAP_GPIO_OUT_OD_50      0x7U    /* CNF=01 MODE=11 开漏输出50MHz */

/**
 * @brief Configure one GPIO pin CNF/MODE nibble (CRL pins 0-7, CRH 8-15).
 */
__STATIC_INLINE void dap_gpio_mode (GPIO_TypeDef *port, uint32_t bit, uint32_t mode) {
  volatile uint32_t *cr;
  uint32_t shift;
  
  cr = (bit >= 8U) ? &port->CRH : &port->CRL; // 判断bit是否大于8，如果大于8，则使用CRH，否则使用CRL
  shift = (bit & 7U) << 2U; // 将bit的低3位左移2位，得到shift
  *cr = (*cr & ~(0xFU << shift)) | (mode << shift); // 将cr的低4位与0xFU左移shift位，然后取反，然后与mode左移shift位，然后或运算，得到新的cr
}

/**
 * @brief Open-drain reset pin: 0 = drive low (assert), 1 = input pull-up.
 */
__STATIC_INLINE void dap_pin_od_out (GPIO_TypeDef *port, uint32_t bit, uint32_t val) {
  uint32_t mask;

  mask = 1U << bit; // 将bit左移1位，得到mask
  if (val != 0U) {
    dap_gpio_mode(port, bit, DAP_GPIO_IN_PU); // 将port的bit位设置为上拉输入
    port->BSRR = mask; // 将port的bit位设置为高电平
  } else {
    dap_gpio_mode(port, bit, DAP_GPIO_OUT_OD_50); // 将port的bit位设置为开漏输出50MHz
    port->BRR = mask; // 将port的bit位设置为低电平
  }
}


// Configure DAP I/O pins ------------------------------

/** Setup JTAG I/O pins: TCK, TMS, TDI, TDO, nTRST, and nRESET.
Configures the DAP Hardware I/O pins for JTAG mode:
 - TCK, TMS, TDI, nTRST, nRESET to output mode and set to high level.
 - TDO to input mode.
*/
__STATIC_INLINE void PORT_JTAG_SETUP (void) {
  PIN_SWCLK_TCK_PORT->BSRR = PIN_SWCLK_TCK_MASK;
  PIN_SWDIO_TMS_PORT->BSRR = PIN_SWDIO_TMS_MASK;
  PIN_TDI_PORT->BSRR       = PIN_TDI_MASK;

  dap_gpio_mode(PIN_SWCLK_TCK_PORT, PIN_SWCLK_TCK_BIT, DAP_GPIO_OUT_PP_50);
  dap_gpio_mode(PIN_SWDIO_TMS_PORT, PIN_SWDIO_TMS_BIT, DAP_GPIO_OUT_PP_50);
  dap_gpio_mode(PIN_TDI_PORT,       PIN_TDI_BIT,       DAP_GPIO_OUT_PP_50);
  dap_gpio_mode(PIN_TDO_PORT,       PIN_TDO_BIT,       DAP_GPIO_IN_FLOAT);

  dap_pin_od_out(PIN_nTRST_PORT,  PIN_nTRST_BIT,  1U);
  dap_pin_od_out(PIN_nRESET_PORT, PIN_nRESET_BIT, 1U);
}

/** Setup SWD I/O pins: SWCLK, SWDIO, and nRESET.
Configures the DAP Hardware I/O pins for Serial Wire Debug (SWD) mode:
 - SWCLK, SWDIO, nRESET to output mode and set to default high level.
 - TDI to HighZ; nTRST idle pull-up (do not float low / assert TRST).
 - JTDO (PA6) High-Z：与 PA10 SWO 短接时只旁听，USART1_RX 收 SWO。
*/
__STATIC_INLINE void PORT_SWD_SETUP (void) {
  PIN_SWCLK_TCK_PORT->BSRR = PIN_SWCLK_TCK_MASK;
  PIN_SWDIO_TMS_PORT->BSRR = PIN_SWDIO_TMS_MASK;

  dap_gpio_mode(PIN_SWCLK_TCK_PORT, PIN_SWCLK_TCK_BIT, DAP_GPIO_OUT_PP_50);
  dap_gpio_mode(PIN_SWDIO_TMS_PORT, PIN_SWDIO_TMS_BIT, DAP_GPIO_OUT_PP_50);
  dap_pin_od_out(PIN_nRESET_PORT, PIN_nRESET_BIT, 1U);
  dap_pin_od_out(PIN_nTRST_PORT,  PIN_nTRST_BIT,  1U);

  dap_gpio_mode(PIN_TDI_PORT, PIN_TDI_BIT, DAP_GPIO_IN_FLOAT);
  dap_gpio_mode(PIN_TDO_PORT, PIN_TDO_BIT, DAP_GPIO_IN_FLOAT);
}

/** Disable JTAG/SWD I/O Pins.
Disables the DAP Hardware I/O pins which configures:
 - TCK/SWCLK, TMS/SWDIO, TDI, TDO, nTRST, nRESET to High-Z mode.
*/
__STATIC_INLINE void PORT_OFF (void) {
  dap_gpio_mode(PIN_SWCLK_TCK_PORT, PIN_SWCLK_TCK_BIT, DAP_GPIO_IN_FLOAT);
  dap_gpio_mode(PIN_SWDIO_TMS_PORT, PIN_SWDIO_TMS_BIT, DAP_GPIO_IN_FLOAT);
  dap_gpio_mode(PIN_TDI_PORT,       PIN_TDI_BIT,       DAP_GPIO_IN_FLOAT);
  dap_gpio_mode(PIN_TDO_PORT,       PIN_TDO_BIT,       DAP_GPIO_IN_FLOAT);
  dap_gpio_mode(PIN_nTRST_PORT,     PIN_nTRST_BIT,     DAP_GPIO_IN_PU);
  PIN_nTRST_PORT->BSRR = PIN_nTRST_MASK;
  dap_gpio_mode(PIN_nRESET_PORT,    PIN_nRESET_BIT,    DAP_GPIO_IN_PU);
  PIN_nRESET_PORT->BSRR = PIN_nRESET_MASK;
}


// SWCLK/TCK I/O pin -------------------------------------

/** SWCLK/TCK I/O pin: Get Input.
\return Current status of the SWCLK/TCK DAP hardware I/O pin.
*/
__STATIC_FORCEINLINE uint32_t PIN_SWCLK_TCK_IN  (void) {
  return ((PIN_SWCLK_TCK_PORT->IDR >> PIN_SWCLK_TCK_BIT) & 1U); //
}

/** SWCLK/TCK I/O pin: Set Output to High.
Set the SWCLK/TCK DAP hardware I/O pin to high level.
*/
__STATIC_FORCEINLINE void     PIN_SWCLK_TCK_SET (void) {
  PIN_SWCLK_TCK_PORT->BSRR = PIN_SWCLK_TCK_MASK;
}

/** SWCLK/TCK I/O pin: Set Output to Low.
Set the SWCLK/TCK DAP hardware I/O pin to low level.
*/
__STATIC_FORCEINLINE void     PIN_SWCLK_TCK_CLR (void) {
  PIN_SWCLK_TCK_PORT->BRR = PIN_SWCLK_TCK_MASK;
}


// SWDIO/TMS Pin I/O --------------------------------------

/** SWDIO/TMS I/O pin: Get Input.
\return Current status of the SWDIO/TMS DAP hardware I/O pin.
*/
__STATIC_FORCEINLINE uint32_t PIN_SWDIO_TMS_IN  (void) {
  return ((PIN_SWDIO_TMS_PORT->IDR >> PIN_SWDIO_TMS_BIT) & 1U);
}

/** SWDIO/TMS I/O pin: Set Output to High.
Set the SWDIO/TMS DAP hardware I/O pin to high level.
*/
__STATIC_FORCEINLINE void     PIN_SWDIO_TMS_SET (void) {
  PIN_SWDIO_TMS_PORT->BSRR = PIN_SWDIO_TMS_MASK;
}

/** SWDIO/TMS I/O pin: Set Output to Low.
Set the SWDIO/TMS DAP hardware I/O pin to low level.
*/
__STATIC_FORCEINLINE void     PIN_SWDIO_TMS_CLR (void) {
  PIN_SWDIO_TMS_PORT->BRR = PIN_SWDIO_TMS_MASK;
}

/** SWDIO I/O pin: Get Input (used in SWD mode only).
\return Current status of the SWDIO DAP hardware I/O pin.
*/
__STATIC_FORCEINLINE uint32_t PIN_SWDIO_IN      (void) {
  return ((PIN_SWDIO_TMS_PORT->IDR >> PIN_SWDIO_TMS_BIT) & 1U);
}

/** SWDIO I/O pin: Set Output (used in SWD mode only).
\param bit Output value for the SWDIO DAP hardware I/O pin.
*/
__STATIC_FORCEINLINE void     PIN_SWDIO_OUT     (uint32_t bit) {
  PIN_SWDIO_TMS_PORT->BSRR = PIN_SWDIO_TMS_MASK << (((bit & 1U) ^ 1U) << 4);
}

/** SWDIO I/O pin: Switch to Output mode (used in SWD mode only).
Configure the SWDIO DAP hardware I/O pin to output mode. This function is
called prior \ref PIN_SWDIO_OUT function calls.
*/
__STATIC_FORCEINLINE void     PIN_SWDIO_OUT_ENABLE  (void) {
  dap_gpio_mode(PIN_SWDIO_TMS_PORT, PIN_SWDIO_TMS_BIT, DAP_GPIO_OUT_PP_50);
}

/** SWDIO I/O pin: Switch to Input mode (used in SWD mode only).
Configure the SWDIO DAP hardware I/O pin to input mode. This function is
called prior \ref PIN_SWDIO_IN function calls.
*/
__STATIC_FORCEINLINE void     PIN_SWDIO_OUT_DISABLE (void) {
  dap_gpio_mode(PIN_SWDIO_TMS_PORT, PIN_SWDIO_TMS_BIT, DAP_GPIO_IN_PU);
  PIN_SWDIO_TMS_PORT->BSRR = PIN_SWDIO_TMS_MASK;
}


// TDI Pin I/O ---------------------------------------------

/** TDI I/O pin: Get Input.
\return Current status of the TDI DAP hardware I/O pin.
*/
__STATIC_FORCEINLINE uint32_t PIN_TDI_IN  (void) {
  return ((PIN_TDI_PORT->IDR >> PIN_TDI_BIT) & 1U);
}

/** TDI I/O pin: Set Output.
\param bit Output value for the TDI DAP hardware I/O pin.
*/
__STATIC_FORCEINLINE void     PIN_TDI_OUT (uint32_t bit) {
  PIN_TDI_PORT->BSRR = PIN_TDI_MASK << (((bit & 1U) ^ 1U) << 4);
}


// TDO Pin I/O ---------------------------------------------

/** TDO I/O pin: Get Input.
\return Current status of the TDO DAP hardware I/O pin.
*/
__STATIC_FORCEINLINE uint32_t PIN_TDO_IN  (void) {
  return ((PIN_TDO_PORT->IDR >> PIN_TDO_BIT) & 1U);
}


// nTRST Pin I/O -------------------------------------------

/** nTRST I/O pin: Get Input.
\return Current status of the nTRST DAP hardware I/O pin.
*/
__STATIC_FORCEINLINE uint32_t PIN_nTRST_IN   (void) {
  return ((PIN_nTRST_PORT->IDR >> PIN_nTRST_BIT) & 1U);
}

/** nTRST I/O pin: Set Output.
\param bit JTAG TRST Test Reset pin status:
           - 0: issue a JTAG TRST Test Reset.
           - 1: release JTAG TRST Test Reset.
*/
__STATIC_FORCEINLINE void     PIN_nTRST_OUT  (uint32_t bit) {
  dap_pin_od_out(PIN_nTRST_PORT, PIN_nTRST_BIT, bit);
}

// nRESET Pin I/O------------------------------------------

/** nRESET I/O pin: Get Input.
\return Current status of the nRESET DAP hardware I/O pin.
*/
__STATIC_FORCEINLINE uint32_t PIN_nRESET_IN  (void) {
  return ((PIN_nRESET_PORT->IDR >> PIN_nRESET_BIT) & 1U);
}

/** nRESET I/O pin: Set Output.
\param bit target device hardware reset pin status:
           - 0: issue a device hardware reset.
           - 1: release device hardware reset.
*/
__STATIC_FORCEINLINE void     PIN_nRESET_OUT (uint32_t bit) {
  dap_pin_od_out(PIN_nRESET_PORT, PIN_nRESET_BIT, bit);
}

///@}


//**************************************************************************************************
/**
\defgroup DAP_Config_LEDs_gr CMSIS-DAP Hardware Status LEDs
\ingroup DAP_ConfigIO_gr
@{

CMSIS-DAP Hardware may provide LEDs that indicate the status of the CMSIS-DAP Debug Unit.

It is recommended to provide the following LEDs for status indication:
 - Connect LED: is active when the DAP hardware is connected to a debugger.
 - Running LED: is active when the debugger has put the target device into running state.
*/

/** Debug Unit: Set status of Connected LED.
\param bit status of the Connect LED.
           - 1: Connect LED ON: debugger is connected to CMSIS-DAP Debug Unit.
           - 0: Connect LED OFF: debugger is not connected to CMSIS-DAP Debug Unit.
*/
__STATIC_INLINE void LED_CONNECTED_OUT (uint32_t bit) {
  PIN_LED_PORT->BSRR = PIN_LED_MASK << (((bit & 1U) ^ 1U) << 4);
}

/** Debug Unit: Set status Target Running LED.
\param bit status of the Target Running LED.
           - 1: Target Running LED ON: program execution in target started.
           - 0: Target Running LED OFF: program execution in target stopped.
*/
__STATIC_INLINE void LED_RUNNING_OUT (uint32_t bit) {
  PIN_LED_PORT->BSRR = PIN_LED_MASK << (((bit & 1U) ^ 1U) << 4);
}

///@}


//**************************************************************************************************
/**
\defgroup DAP_Config_Timestamp_gr CMSIS-DAP Timestamp
\ingroup DAP_ConfigIO_gr
@{
Access function for Test Domain Timer.

The value of the Test Domain Timer in the Debug Unit is returned by the function \ref TIMESTAMP_GET. By
default, the DWT timer is used.  The frequency of this timer is configured with \ref TIMESTAMP_CLOCK.

*/

/** Get timestamp of Test Domain Timer.
\return Current timestamp value.
*/
__STATIC_INLINE uint32_t TIMESTAMP_GET (void) {
  return (DWT->CYCCNT);
}

///@}


//**************************************************************************************************
/**
\defgroup DAP_Config_Initialization_gr CMSIS-DAP Initialization
\ingroup DAP_ConfigIO_gr
@{

CMSIS-DAP Hardware I/O and LED Pins are initialized with the function \ref DAP_SETUP.
*/

/** Setup of the Debug Unit I/O pins and LEDs (called when Debug Unit is initialized).
This function performs the initialization of the CMSIS-DAP Hardware I/O Pins and the
Status LEDs. In detail the operation of Hardware I/O and LED pins are enabled and set:
 - I/O clock system enabled.
 - all I/O pins: input buffer enabled, output pins are set to HighZ mode.
 - for nTRST, nRESET a weak pull-up (if available) is enabled.
 - LED output pins are enabled and LEDs are turned off.
*/
__STATIC_INLINE void DAP_SETUP (void) {
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
  (void)RCC->APB2ENR;

  PORT_OFF();

  dap_gpio_mode(PIN_LED_PORT, PIN_LED_BIT, DAP_GPIO_OUT_PP_50);
  PIN_LED_PORT->BRR = PIN_LED_MASK;
}

/** Reset Target Device with custom specific I/O pin or command sequence.
This function allows the optional implementation of a device specific reset sequence.
It is called when the command \ref DAP_ResetTarget and is for example required
when a device needs a time-critical unlock sequence that enables the debug port.
\return 0 = no device specific reset sequence is implemented.\n
        1 = a device specific reset sequence is implemented.
*/
__STATIC_INLINE uint8_t RESET_TARGET (void) {
  return (0U);             // change to '1' when a device reset sequence is implemented
}

///@}


#endif /* __DAP_CONFIG_H__ */

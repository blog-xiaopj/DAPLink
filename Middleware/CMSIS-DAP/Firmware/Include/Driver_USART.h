/*
 * Minimal CMSIS-Driver USART definitions for CMSIS-DAP SWO UART.
 * Subset of ARM CMSIS Driver_USART.h (Apache-2.0).
 */
#ifndef DRIVER_USART_H_
#define DRIVER_USART_H_

#include "Driver_Common.h"

#define ARM_USART_API_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(2, 4)

/****** USART Control Codes *****/

#define ARM_USART_CONTROL_Pos 0
#define ARM_USART_CONTROL_Msk (0xFFUL << ARM_USART_CONTROL_Pos)

/*----- USART Control Codes: Mode -----*/
#define ARM_USART_MODE_ASYNCHRONOUS (0x01UL << ARM_USART_CONTROL_Pos)

/*----- USART Control Codes: Mode Parameters: Data Bits -----*/
#define ARM_USART_DATA_BITS_Pos 8
#define ARM_USART_DATA_BITS_Msk (7UL << ARM_USART_DATA_BITS_Pos)
#define ARM_USART_DATA_BITS_8   (0UL << ARM_USART_DATA_BITS_Pos)

/*----- USART Control Codes: Mode Parameters: Parity -----*/
#define ARM_USART_PARITY_Pos  12
#define ARM_USART_PARITY_Msk  (3UL << ARM_USART_PARITY_Pos)
#define ARM_USART_PARITY_NONE (0UL << ARM_USART_PARITY_Pos)

/*----- USART Control Codes: Mode Parameters: Stop Bits -----*/
#define ARM_USART_STOP_BITS_Pos 14
#define ARM_USART_STOP_BITS_Msk (3UL << ARM_USART_STOP_BITS_Pos)
#define ARM_USART_STOP_BITS_1   (0UL << ARM_USART_STOP_BITS_Pos)

/*----- USART Control Codes: Miscellaneous Controls -----*/
#define ARM_USART_CONTROL_TX    (0x15UL << ARM_USART_CONTROL_Pos)
#define ARM_USART_CONTROL_RX    (0x16UL << ARM_USART_CONTROL_Pos)
#define ARM_USART_ABORT_RECEIVE (0x19UL << ARM_USART_CONTROL_Pos)

/****** USART Event *****/
#define ARM_USART_EVENT_SEND_COMPLETE     (1UL << 0)
#define ARM_USART_EVENT_RECEIVE_COMPLETE  (1UL << 1)
#define ARM_USART_EVENT_TRANSFER_COMPLETE (1UL << 2)
#define ARM_USART_EVENT_TX_COMPLETE       (1UL << 3)
#define ARM_USART_EVENT_TX_UNDERFLOW      (1UL << 4)
#define ARM_USART_EVENT_RX_OVERFLOW       (1UL << 5)
#define ARM_USART_EVENT_RX_TIMEOUT        (1UL << 6)
#define ARM_USART_EVENT_RX_BREAK          (1UL << 7)
#define ARM_USART_EVENT_RX_FRAMING_ERROR  (1UL << 8)
#define ARM_USART_EVENT_RX_PARITY_ERROR   (1UL << 9)
#define ARM_USART_EVENT_CTS               (1UL << 10)
#define ARM_USART_EVENT_DSR               (1UL << 11)
#define ARM_USART_EVENT_DCD               (1UL << 12)
#define ARM_USART_EVENT_RI                (1UL << 13)

/**
\brief USART Status
*/
typedef struct _ARM_USART_STATUS {
	uint32_t tx_busy : 1;
	uint32_t rx_busy : 1;
	uint32_t tx_underflow : 1;
	uint32_t rx_overflow : 1;
	uint32_t rx_break : 1;
	uint32_t rx_framing_error : 1;
	uint32_t rx_parity_error : 1;
	uint32_t reserved : 25;
} ARM_USART_STATUS;

/**
\brief USART Modem Control
*/
typedef enum _ARM_USART_MODEM_CONTROL {
	ARM_USART_RTS_CLEAR,
	ARM_USART_RTS_SET,
	ARM_USART_DTR_CLEAR,
	ARM_USART_DTR_SET
} ARM_USART_MODEM_CONTROL;

/**
\brief USART Modem Status
*/
typedef struct _ARM_USART_MODEM_STATUS {
	uint32_t cts : 1;
	uint32_t dsr : 1;
	uint32_t dcd : 1;
	uint32_t ri : 1;
	uint32_t reserved : 28;
} ARM_USART_MODEM_STATUS;

/**
\brief USART Device Driver Capabilities.
*/
typedef struct _ARM_USART_CAPABILITIES {
	uint32_t asynchronous : 1;
	uint32_t synchronous_master : 1;
	uint32_t synchronous_slave : 1;
	uint32_t single_wire : 1;
	uint32_t irda : 1;
	uint32_t smart_card : 1;
	uint32_t smart_card_clock : 1;
	uint32_t flow_control_rts : 1;
	uint32_t flow_control_cts : 1;
	uint32_t event_tx_complete : 1;
	uint32_t event_rx_timeout : 1;
	uint32_t rts : 1;
	uint32_t cts : 1;
	uint32_t dtr : 1;
	uint32_t dsr : 1;
	uint32_t dcd : 1;
	uint32_t ri : 1;
	uint32_t event_cts : 1;
	uint32_t event_dsr : 1;
	uint32_t event_dcd : 1;
	uint32_t event_ri : 1;
	uint32_t reserved : 11;
} ARM_USART_CAPABILITIES;

typedef void (*ARM_USART_SignalEvent_t)(uint32_t event);

/**
\brief Access structure of the USART Driver.
*/
typedef struct _ARM_DRIVER_USART {
	ARM_DRIVER_VERSION (*GetVersion)(void);
	ARM_USART_CAPABILITIES (*GetCapabilities)(void);
	int32_t (*Initialize)(ARM_USART_SignalEvent_t cb_event);
	int32_t (*Uninitialize)(void);
	int32_t (*PowerControl)(ARM_POWER_STATE state);
	int32_t (*Send)(const void *data, uint32_t num);
	int32_t (*Receive)(void *data, uint32_t num);
	int32_t (*Transfer)(const void *data_out, void *data_in, uint32_t num);
	uint32_t (*GetTxCount)(void);
	uint32_t (*GetRxCount)(void);
	int32_t (*Control)(uint32_t control, uint32_t arg);
	ARM_USART_STATUS (*GetStatus)(void);
	int32_t (*SetModemControl)(ARM_USART_MODEM_CONTROL control);
	ARM_USART_MODEM_STATUS (*GetModemStatus)(void);
} const ARM_DRIVER_USART;

#endif /* DRIVER_USART_H_ */

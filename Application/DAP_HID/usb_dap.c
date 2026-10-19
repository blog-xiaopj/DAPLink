/**
 * @file    usb_dap.c
 * @brief   CMSIS-DAP USB 传输（HID）+ 两路 CDC ACM 虚拟串口
 */
#include "usb_dap.h"

#include <string.h>

#include "stm32f10x.h"

#include "DAP.h"
#include "DAP_config.h"

#include "usbd_core.h"
#include "usbd_cdc_acm.h"
#include "usbd_hid.h"

#include "FreeRTOS.h"
#include "message_buffer.h"
#include "semphr.h"
#include "stream_buffer.h"
#include "task.h"

#define BUSID 0

#define DAP_IN_EP 0x81
#define DAP_OUT_EP 0x02

#define CDC_IN_EP 0x83
#define CDC_OUT_EP 0x04
#define CDC_INT_EP 0x85

#define CDC2_IN_EP 0x87
#define CDC2_OUT_EP 0x07
#define CDC2_INT_EP 0x86

#define HID_INT_INTERVAL 1U
/* STM32F103 PMA 512B：两路 64B CDC 会溢出，故降为 32B */
#define CDC_MAX_MPS 32U

#define USBD_VID 0x0D28
#define USBD_PID 0x0204 /* CMSIS-DAP v1 HID，与 WinUSB 固件 PID 分开以免 Windows 缓存驱动 */
#define USBD_MAX_POWER 100
#define USB_CONFIG_SIZE (9 + HID_CUSTOM_INOUT_DESCRIPTOR_LEN + CDC_ACM_DESCRIPTOR_LEN * 2)
#define INTF_NUM 5

#define HID_REPORT_DESC_SIZE 33U

#define DAP_TASK_STACK 128U
#define DAP_TASK_PRIO 4U
#define CDC_TASK_STACK 128U
#define CDC_TASK_PRIO 3U
#define CDC_STREAM_SIZE 1024U

static SemaphoreHandle_t s_semaphore_write;
static SemaphoreHandle_t s_cdc_semaphore_write;
static StreamBufferHandle_t s_cdc_read_buffer;
static StreamBufferHandle_t s_cdc_write_buffer;

static SemaphoreHandle_t s_cdc2_semaphore_write;
static StreamBufferHandle_t s_cdc2_read_buffer;
static StreamBufferHandle_t s_cdc2_write_buffer;

static const uint8_t hid_report_desc[HID_REPORT_DESC_SIZE] = {
	0x06, 0x00, 0xFF, /* Usage Page (Vendor Defined 0xFF00) */
	0x09, 0x01,		  /* Usage (0x01) */
	0xA1, 0x01,		  /* Collection (Application) */
	0x15, 0x00,		  /*   Logical Minimum (0) */
	0x26, 0xFF, 0x00, /*   Logical Maximum (255) */
	0x75, 0x08,		  /*   Report Size (8) */
	0x95, 0x40,		  /*   Report Count (64) */
	0x09, 0x01,		  /*   Usage (0x01) */
	0x81, 0x02,		  /*   Input (Data,Var,Abs) */
	0x95, 0x40,		  /*   Report Count (64) */
	0x09, 0x01,		  /*   Usage (0x01) */
	0x91, 0x02,		  /*   Output (Data,Var,Abs) */
	0x95, 0x01,		  /*   Report Count (1) */
	0x09, 0x01,		  /*   Usage (0x01) */
	0xB1, 0x02,		  /*   Feature (Data,Var,Abs) */
	0xC0			  /* End Collection */
};

static const uint8_t device_descriptor[] = {USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0101, 0x01)};

static const uint8_t config_descriptor[] = {
	USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, INTF_NUM, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
	HID_CUSTOM_INOUT_DESCRIPTOR_INIT(0x00, 0x00, HID_REPORT_DESC_SIZE, DAP_OUT_EP, DAP_IN_EP, DAP_PACKET_SIZE, HID_INT_INTERVAL),
	CDC_ACM_DESCRIPTOR_INIT(0x01, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, CDC_MAX_MPS, 0x04),
	CDC_ACM_DESCRIPTOR_INIT(0x03, CDC2_INT_EP, CDC2_OUT_EP, CDC2_IN_EP, CDC_MAX_MPS, 0x05),
};

static const uint8_t device_quality_descriptor[] = {
	USB_DEVICE_QUALIFIER_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, 0x01),
};

static const char *string_descriptors[] = {
	(const char[]){0x09, 0x04},
	"CherryUSB",
	"CMSIS-DAP V1",
	"xiaopj.com",
	"UART1 RS232",
	"UART2 RS485",
};

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
	(void)speed;
	return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
	(void)speed;
	return config_descriptor;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
	(void)speed;
	return device_quality_descriptor;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
	(void)speed;
	if (index >= (sizeof(string_descriptors) / sizeof(string_descriptors[0])))
		return NULL;
	return string_descriptors[index];
}

static const struct usb_descriptor s_cmsisdap_descriptor = {
	.device_descriptor_callback = device_descriptor_callback,
	.config_descriptor_callback = config_descriptor_callback,
	.device_quality_descriptor_callback = device_quality_descriptor_callback,
	.string_descriptor_callback = string_descriptor_callback,
};

static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t s_usb_request[DAP_PACKET_SIZE];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t s_usb_response[DAP_PACKET_SIZE];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t s_cdc_rx_buf[CDC_MAX_MPS];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t s_cdc_tx_buf[CDC_MAX_MPS];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t s_cdc2_rx_buf[CDC_MAX_MPS];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t s_cdc2_tx_buf[CDC_MAX_MPS];

static StreamBufferHandle_t s_req_buffer;
static StreamBufferHandle_t s_resp_buffer;

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
	(void)busid;

	switch (event) {
	case USBD_EVENT_CONFIGURED: {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(s_semaphore_write, &xHigherPriorityTaskWoken);
		xSemaphoreGiveFromISR(s_cdc_semaphore_write, &xHigherPriorityTaskWoken);
		xSemaphoreGiveFromISR(s_cdc2_semaphore_write, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		usbd_ep_start_read(BUSID, DAP_OUT_EP, s_usb_request, DAP_PACKET_SIZE);
		usbd_ep_start_read(BUSID, CDC_OUT_EP, s_cdc_rx_buf, CDC_MAX_MPS);
		usbd_ep_start_read(BUSID, CDC2_OUT_EP, s_cdc2_rx_buf, CDC_MAX_MPS);
	} break;
	default:
		break;
	}
}

static void dap_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
	(void)busid;
	(void)ep;

	if (s_usb_request[0] == ID_DAP_TransferAbort)
		DAP_TransferAbort = 1U;
	else {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xMessageBufferSendFromISR(s_req_buffer, s_usb_request, nbytes, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	usbd_ep_start_read(BUSID, DAP_OUT_EP, s_usb_request, DAP_PACKET_SIZE);
}

static void dap_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
	(void)busid;
	(void)ep;
	(void)nbytes;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(s_semaphore_write, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void cdc_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
	(void)ep;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	xStreamBufferSendFromISR(s_cdc_read_buffer, s_cdc_rx_buf, nbytes, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	usbd_ep_start_read(busid, CDC_OUT_EP, s_cdc_rx_buf, CDC_MAX_MPS);
}

static void cdc_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
	if ((nbytes % usbd_get_ep_mps(busid, ep)) == 0U && nbytes) {
		usbd_ep_start_write(busid, CDC_IN_EP, NULL, 0);
	} else {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(s_cdc_semaphore_write, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

static void cdc2_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
	(void)ep;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	xStreamBufferSendFromISR(s_cdc2_read_buffer, s_cdc2_rx_buf, nbytes, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	usbd_ep_start_read(busid, CDC2_OUT_EP, s_cdc2_rx_buf, CDC_MAX_MPS);
}

static void cdc2_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
	if ((nbytes % usbd_get_ep_mps(busid, ep)) == 0U && nbytes) {
		usbd_ep_start_write(busid, CDC2_IN_EP, NULL, 0);
	} else {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(s_cdc2_semaphore_write, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

static struct usbd_endpoint s_dap_out_ep = {
	.ep_addr = DAP_OUT_EP,
	.ep_cb = dap_out_callback,
};

static struct usbd_endpoint s_dap_in_ep = {
	.ep_addr = DAP_IN_EP,
	.ep_cb = dap_in_callback,
};

static struct usbd_endpoint s_cdc_out_ep = {
	.ep_addr = CDC_OUT_EP,
	.ep_cb = cdc_out_callback,
};

static struct usbd_endpoint s_cdc_in_ep = {
	.ep_addr = CDC_IN_EP,
	.ep_cb = cdc_in_callback,
};

static struct usbd_endpoint s_cdc2_out_ep = {
	.ep_addr = CDC2_OUT_EP,
	.ep_cb = cdc2_out_callback,
};

static struct usbd_endpoint s_cdc2_in_ep = {
	.ep_addr = CDC2_IN_EP,
	.ep_cb = cdc2_in_callback,
};

static struct usbd_interface s_dap_intf;
static struct usbd_interface s_cdc_intf0;
static struct usbd_interface s_cdc_intf1;
static struct usbd_interface s_cdc2_intf0;
static struct usbd_interface s_cdc2_intf1;

static uint8_t s_response[DAP_PACKET_SIZE];
__WEAK void command_received(uint8_t *request, size_t size)
{
	(void)size;
	if (request[0] == ID_DAP_QueueCommands)
		request[0] = ID_DAP_ExecuteCommands;
	DAP_ExecuteCommand(request, s_response);
	xMessageBufferSend(s_resp_buffer, s_response, DAP_PACKET_SIZE, pdMS_TO_TICKS(10));
}

static void dap_task(void *param)
{
	static uint8_t buffer[DAP_PACKET_SIZE];

	(void)param;
	static uint8_t write_busy = 1;
	while (1) {
		size_t xBytesReceived = xMessageBufferReceive(s_req_buffer, buffer, DAP_PACKET_SIZE, pdMS_TO_TICKS(1));
		if (xBytesReceived > 0)
			command_received(buffer, xBytesReceived);

		if (write_busy) {
			if (xSemaphoreTake(s_semaphore_write, pdMS_TO_TICKS(1)) == pdFAIL)
				continue;
			write_busy = 0;
		}

		xBytesReceived = xMessageBufferReceive(s_resp_buffer, s_usb_response, DAP_PACKET_SIZE, pdMS_TO_TICKS(0));
		if (xBytesReceived > 0) {
			write_busy = 1;
			usbd_ep_start_write(BUSID, DAP_IN_EP, s_usb_response, DAP_PACKET_SIZE);
		}
	}
}

__WEAK void usbd_cdc_acm_received(const char *pBuffer, size_t size)
{
	(void)pBuffer;
	(void)size;
}

void usbd_cdc_acm_transfer(const char *pBuffer, size_t size)
{
	if ((pBuffer == NULL) || (size == 0U) || (s_cdc_write_buffer == NULL))
		return;
	xStreamBufferSend(s_cdc_write_buffer, pBuffer, size, pdMS_TO_TICKS(10));
}

static void cdc_task(void *param)
{
	static char buffer[CDC_MAX_MPS];
	static uint8_t write_busy = 1;

	(void)param;
	while (1) {
		size_t xBytesReceived = xStreamBufferReceive(s_cdc_read_buffer, buffer, CDC_MAX_MPS, pdMS_TO_TICKS(1));
		if (xBytesReceived > 0)
			usbd_cdc_acm_received(buffer, xBytesReceived);

		if (write_busy) {
			if (xSemaphoreTake(s_cdc_semaphore_write, pdMS_TO_TICKS(0)) == pdFAIL)
				continue;
			write_busy = 0;
		}

		xBytesReceived = xStreamBufferReceive(s_cdc_write_buffer, s_cdc_tx_buf, CDC_MAX_MPS, pdMS_TO_TICKS(0));
		if (xBytesReceived > 0) {
			write_busy = 1;
			usbd_ep_start_write(BUSID, CDC_IN_EP, s_cdc_tx_buf, xBytesReceived);
		}
	}
}

__WEAK void usbd_cdc_acm2_received(const char *pBuffer, size_t size)
{
	(void)pBuffer;
	(void)size;
}

void usbd_cdc_acm2_transfer(const char *pBuffer, size_t size)
{
	if ((pBuffer == NULL) || (size == 0U) || (s_cdc2_write_buffer == NULL))
		return;
	xStreamBufferSend(s_cdc2_write_buffer, pBuffer, size, pdMS_TO_TICKS(10));
}

static void cdc2_task(void *param)
{
	static char buffer[CDC_MAX_MPS];
	static uint8_t write_busy = 1;

	(void)param;
	while (1) {
		size_t xBytesReceived = xStreamBufferReceive(s_cdc2_read_buffer, buffer, CDC_MAX_MPS, pdMS_TO_TICKS(1));
		if (xBytesReceived > 0)
			usbd_cdc_acm2_received(buffer, xBytesReceived);

		if (write_busy) {
			if (xSemaphoreTake(s_cdc2_semaphore_write, pdMS_TO_TICKS(0)) == pdFAIL)
				continue;
			write_busy = 0;
		}

		xBytesReceived = xStreamBufferReceive(s_cdc2_write_buffer, s_cdc2_tx_buf, CDC_MAX_MPS, pdMS_TO_TICKS(0));
		if (xBytesReceived > 0) {
			write_busy = 1;
			usbd_ep_start_write(BUSID, CDC2_IN_EP, s_cdc2_tx_buf, xBytesReceived);
		}
	}
}

void usb_dap_init(void)
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0U;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	DAP_Setup();

	s_semaphore_write = xSemaphoreCreateBinary();
	s_cdc_semaphore_write = xSemaphoreCreateBinary();
	s_cdc2_semaphore_write = xSemaphoreCreateBinary();
	s_req_buffer = xMessageBufferCreate((sizeof(size_t) + DAP_PACKET_SIZE) * DAP_PACKET_COUNT);
	s_resp_buffer = xMessageBufferCreate((sizeof(size_t) + DAP_PACKET_SIZE) * DAP_PACKET_COUNT);
	s_cdc_read_buffer = xStreamBufferCreate(CDC_STREAM_SIZE, 1U);
	s_cdc_write_buffer = xStreamBufferCreate(CDC_STREAM_SIZE, 1U);
	s_cdc2_read_buffer = xStreamBufferCreate(CDC_STREAM_SIZE, 1U);
	s_cdc2_write_buffer = xStreamBufferCreate(CDC_STREAM_SIZE, 1U);

	usbd_desc_register(BUSID, &s_cmsisdap_descriptor);
	usbd_add_interface(BUSID, usbd_hid_init_intf(BUSID, &s_dap_intf, hid_report_desc, HID_REPORT_DESC_SIZE));
	usbd_add_endpoint(BUSID, &s_dap_out_ep);
	usbd_add_endpoint(BUSID, &s_dap_in_ep);
	usbd_add_interface(BUSID, usbd_cdc_acm_init_intf(BUSID, &s_cdc_intf0));
	usbd_add_interface(BUSID, usbd_cdc_acm_init_intf(BUSID, &s_cdc_intf1));
	usbd_add_endpoint(BUSID, &s_cdc_out_ep);
	usbd_add_endpoint(BUSID, &s_cdc_in_ep);
	usbd_add_interface(BUSID, usbd_cdc_acm_init_intf(BUSID, &s_cdc2_intf0));
	usbd_add_interface(BUSID, usbd_cdc_acm_init_intf(BUSID, &s_cdc2_intf1));
	usbd_add_endpoint(BUSID, &s_cdc2_out_ep);
	usbd_add_endpoint(BUSID, &s_cdc2_in_ep);
	usbd_initialize(BUSID, USB_BASE, usbd_event_handler);

	xTaskCreate(dap_task, "dap", DAP_TASK_STACK, NULL, DAP_TASK_PRIO, NULL);
	xTaskCreate(cdc_task, "cdc", CDC_TASK_STACK, NULL, CDC_TASK_PRIO, NULL);
	xTaskCreate(cdc2_task, "cdc2", CDC_TASK_STACK, NULL, CDC_TASK_PRIO, NULL);
}

/*
 * Minimal CMSIS-Driver Common definitions for CMSIS-DAP SWO UART.
 * Subset of ARM CMSIS Driver_Common.h (Apache-2.0).
 */
#ifndef DRIVER_COMMON_H_
#define DRIVER_COMMON_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define ARM_DRIVER_VERSION_MAJOR_MINOR(major, minor) (((major) << 8) | (minor))

/**
\brief Driver Version
*/
typedef struct _ARM_DRIVER_VERSION {
	uint16_t api; /**< API version */
	uint16_t drv; /**< Driver version */
} ARM_DRIVER_VERSION;

/* General return codes */
#define ARM_DRIVER_OK                 0  /**< Operation succeeded */
#define ARM_DRIVER_ERROR             -1  /**< Unspecified error */
#define ARM_DRIVER_ERROR_BUSY        -2  /**< Driver is busy */
#define ARM_DRIVER_ERROR_TIMEOUT     -3  /**< Timeout occurred */
#define ARM_DRIVER_ERROR_UNSUPPORTED -4  /**< Operation not supported */
#define ARM_DRIVER_ERROR_PARAMETER   -5  /**< Parameter error */
#define ARM_DRIVER_ERROR_SPECIFIC    -6  /**< Start of driver specific errors */

/**
\brief General power states
*/
typedef enum _ARM_POWER_STATE {
	ARM_POWER_OFF = 0, /**< Power off: no operation possible */
	ARM_POWER_LOW = 1, /**< Low Power mode: retain state, detect and signal wake-up events */
	ARM_POWER_FULL = 2 /**< Power on: full operation at maximum performance */
} ARM_POWER_STATE;

#endif /* DRIVER_COMMON_H_ */

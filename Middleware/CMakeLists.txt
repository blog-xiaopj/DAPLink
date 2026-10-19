# CMSIS
add_library(cmsis STATIC
	# CMSIS/CM3/CoreSupport/core_cm3.c
	CMSIS/CM3/DeviceSupport/ST/STM32F10x/system_stm32f10x.c
)
target_include_directories(cmsis PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Include
	${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/CM3/DeviceSupport/ST/STM32F10x
	${CMAKE_CURRENT_SOURCE_DIR}/STM32F10x_StdPeriph_Driver/inc
)

# STM32F10x 标准外设库
aux_source_directory(STM32F10x_StdPeriph_Driver/src STD_LIST)
add_library(stdperiph STATIC ${STD_LIST})
target_include_directories(stdperiph PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/STM32F10x_StdPeriph_Driver/inc
)
target_link_libraries(stdperiph PUBLIC cmsis)

# FreeRTOS Kernel（不使用官方 CMake，避免依赖 freertos_config / FREERTOS_PORT）
add_library(freertos STATIC
	FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c
	FreeRTOS-Kernel/portable/MemMang/heap_4.c
	FreeRTOS-Kernel/croutine.c
	FreeRTOS-Kernel/event_groups.c
	FreeRTOS-Kernel/list.c
	FreeRTOS-Kernel/queue.c
	FreeRTOS-Kernel/stream_buffer.c
	FreeRTOS-Kernel/tasks.c
	FreeRTOS-Kernel/timers.c
)
target_include_directories(freertos PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel
	${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/include
	# ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/examples/template_configuration
	${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/portable/GCC/ARM_CM3
)
target_link_libraries(freertos PUBLIC cmsis)

# Cherry-USB
add_library(cherry_usb STATIC
	CherryUSB/core/usbd_core.c
	CherryUSB/port/fsdev/usb_dc_fsdev.c
	CherryUSB/class/cdc/usbd_cdc_acm.c
	CherryUSB/class/hid/usbd_hid.c
)
target_include_directories(cherry_usb PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/CherryUSB
	${CMAKE_CURRENT_SOURCE_DIR}/CherryUSB/core
	${CMAKE_CURRENT_SOURCE_DIR}/CherryUSB/common
	${CMAKE_CURRENT_SOURCE_DIR}/CherryUSB/port/fsdev
	${CMAKE_CURRENT_SOURCE_DIR}/CherryUSB/class/cdc
	${CMAKE_CURRENT_SOURCE_DIR}/CherryUSB/class/hid
)
target_link_libraries(cherry_usb PUBLIC cmsis)

# CMSIS-DAP
add_library(cmsis_dap STATIC
	CMSIS-DAP/Firmware/Source/DAP.c
	CMSIS-DAP/Firmware/Source/JTAG_DP.c
	CMSIS-DAP/Firmware/Source/SW_DP.c
	CMSIS-DAP/Firmware/Source/SWO.c
	CMSIS-DAP/Firmware/Source/UART.c
)

target_include_directories(cmsis_dap PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/CMSIS-DAP/Firmware/Include
	${CMAKE_CURRENT_SOURCE_DIR}/CMSIS-DAP/Firmware/Config
)
target_link_libraries(cmsis_dap PUBLIC cmsis)






if(BOARD_TYPE STREQUAL "RTT")
	add_definitions(-DRTT)
	message("build for RTT!")
	add_subdirectory(RTT)
endif()

if(BOARD_TYPE STREQUAL "DAP_HID")
	add_definitions(-DDAP_HID)
	message("build for DAP_HID!")
	add_subdirectory(DAP_HID)
endif()

if(BOARD_TYPE STREQUAL "DAP_WINUSB")
	add_definitions(-DDAP_WINUSB)
	message("build for DAP_WINUSB!")
	add_subdirectory(DAP_WINUSB)
endif()

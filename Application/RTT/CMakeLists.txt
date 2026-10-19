set(VERSION_MAJOR "0")
set(VERSION_MINOR "0")
set(VERSION_PATCH "1")

# v0.0.1 LED + RTT / SWO(ITM) 双通道打印

add_definitions(-DVERSION_MAJOR=${VERSION_MAJOR})
add_definitions(-DVERSION_MINOR=${VERSION_MINOR})
add_definitions(-DVERSION_PATCH=${VERSION_PATCH})

# 启动文件
set(START_UP_ASM
	${PROJECT_SOURCE_DIR}/Middleware/CMSIS/CM3/DeviceSupport/ST/STM32F10x/startup/gcc_ride7/startup_stm32f10x_md.s
)

# 设置支持 ASM
enable_language(ASM)

# 设置启动文件 C 属性
set_property(SOURCE ${START_UP_ASM} PROPERTY LANGUAGE C)

# 设置链接脚本路径
set(LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/script/STM32F103C8T6.ld)

# 设置链接器选项（不用 rdimon，printf 走 RTT/SWO 而非 semihosting）
set(CMAKE_EXE_LINKER_FLAGS
	" --specs=nano.specs -specs=nosys.specs -T${LINKER_SCRIPT} -Wl,-Map=${PROJECT_BINARY_DIR}/firmware.map,--cref -Wl,--gc-sections -Wl,--print-memory-usage"
)

# 设置可执行文件输出路径（须在 add_executable 之前）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

# 生成目标文件
add_executable(firmware.elf
	main.c
	rtt.c
	swo.c
	uart2.c
	${START_UP_ASM}
)

target_include_directories(firmware.elf PRIVATE

)

target_link_libraries(firmware.elf freertos stdperiph cmsis cherry_usb)

# 设置 ELF 转换路径
set(ELF_FILE ${PROJECT_BINARY_DIR}/firmware.elf)
set(HEX_FILE ${PROJECT_BINARY_DIR}/firmware.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/firmware.bin)

add_custom_command(TARGET "firmware.elf" POST_BUILD
	COMMAND ${CMAKE_OBJCOPY} -Obinary ${ELF_FILE} ${BIN_FILE}
	COMMAND ${CMAKE_OBJCOPY} -Oihex ${ELF_FILE} ${HEX_FILE}
	COMMAND ${CMAKE_COMMAND} -E copy ${HEX_FILE} "${PROJECT_BINARY_DIR}/bin/TEST_RTT.hex"
	COMMAND ${CMAKE_COMMAND} -E copy ${BIN_FILE} "${PROJECT_BINARY_DIR}/bin/TEST_RTT.bin"
	COMMAND ${CMAKE_COMMAND} -E copy ${ELF_FILE} "${PROJECT_BINARY_DIR}/bin/TEST_RTT.elf"
	COMMAND ${CMAKE_SIZE} --format=berkeley ${ELF_FILE} ${HEX_FILE}
	COMMENT "Building ${PROJECT_NAME} (bin, hex, elf) and print size"
)


# 设置 CMake 最低支持版本
cmake_minimum_required(VERSION 3.17)

# 导出编译命令
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Cmake 交叉编译配置
set(CMAKE_SYSTEM_NAME Generic)

# 定义工程名称
project("CHERRY_USB")

# 指定编译工具
set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-none-eabi-g++")
set(CMAKE_ASM_COMPILER "arm-none-eabi-gcc")
set(CMAKE_AR "arm-none-eabi-ar")
set(CMAKE_OBJCOPY "arm-none-eabi-objcopy")
set(CMAKE_OBJDUMP "arm-none-eabi-objdump")
set(CMAKE_SIZE "arm-none-eabi-size")



# 编译相关选项
set(MCU_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft") 
set(CMAKE_C_FLAGS_DEBUG "-g -ggdb -Og")
set(CMAKE_C_FLAGS_RELEASE "-O3")
# set(CMAKE_C_FLAGS "${MCU_FLAGS}  -ffunction-sections -fdata-sections -fno-builtin -fno-common -Wextra -Werror -Wno-unknown-pragmas -Wl,-u,_printf_float") #-w -Wall
set(CMAKE_C_FLAGS "${MCU_FLAGS} -ffunction-sections -fdata-sections -fno-builtin -fno-common ") # -Wall -Wno-unknown-pragmas
set(CMAKE_ASM_FLAGS "${MCU_FLAGS} -x assembler-with-cpp")


if(RELEASE)
    message("build for release!")
    set(CMAKE_BUILD_TYPE "Release")
    add_definitions(-DRELEASE)
else()
    message("build for debug!")
    set(CMAKE_BUILD_TYPE "Debug")
    add_definitions(-DDEBUG)
endif()


# 设置编译器选项
add_definitions(-DUSE_STDPERIPH_DRIVER)
add_definitions(-DSTM32F10X_MD)


add_subdirectory(Middleware) #库
add_subdirectory(Application) #应用

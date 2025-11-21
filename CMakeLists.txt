cmake_minimum_required(VERSION 3.14)

set(PICO_PLATFORM rp2040)
set(PICO_BOARD pico_gmm7550)
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src/include)

set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(gmm7550-control VERSION 0.1)

pico_sdk_init()

set(TARGET_NAME gmm_control)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM
  INTERFACE src/include
  )

set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/freertos-kernel)
set(FREERTOS_PORT GCC_RP2040)
set(FREERTOS_HEAP 4)
add_subdirectory(${FREERTOS_KERNEL_PATH})
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

add_library(tinyusb_config INTERFACE)
target_include_directories(tinyusb_config SYSTEM
  INTERFACE src/include
  )

set(TINYUSB_PATH ${PICO_SDK_PATH}/lib/tinyusb)
include(${TINYUSB_PATH}/src/CMakeLists.txt)

target_link_libraries(tinyusb PRIVATE freertos_kernel)

target_compile_definitions(tinyusb PRIVATE
    PICO_DIVIDER_DISABLE_INTERRUPTS=0
    configUSE_DYNAMIC_EXCEPTION_HANDLERS=0
    configTICK_CORE=0
    LIB_PICO_PRINTF_NONE=1
    LIB_PICO_PRINTF_PICO=0
    PICO_USE_SW_SPIN_LOCKS=0
    )

add_executable(${TARGET_NAME}
    src/main.c
    src/uart.c
    )

target_compile_definitions(${TARGET_NAME} PRIVATE
    configNUMBER_OF_CORES=1
    )

target_link_libraries(${TARGET_NAME} PRIVATE
    pico_stdlib
    freertos_kernel
    tinyusb
    tinyusb_bsp
    tinyusb_device
    )

pico_add_extra_outputs(${TARGET_NAME})

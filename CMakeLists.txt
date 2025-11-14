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

add_executable(${TARGET_NAME}
    src/main.c
    )

target_compile_definitions(${TARGET_NAME} PRIVATE
    configNUMBER_OF_CORES=1
    )

target_link_libraries(${TARGET_NAME} PRIVATE
    pico_stdlib
    freertos_kernel
    )

pico_add_extra_outputs(${TARGET_NAME})

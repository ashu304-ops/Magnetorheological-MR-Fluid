cmake_minimum_required(VERSION 3.20)

project(Integration C CXX ASM)

# ============================================================
# Language standards
# ============================================================

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# ============================================================
# Normal application sources
# ============================================================

set(PROJECT_SOURCES
    src/CoilController.cpp
    src/CoilDriver.cpp
    src/ComfortStrategy.cpp
    src/DampingCalculator.cpp
    src/SensorReader.cpp
    src/SportStrategy.cpp
    src/SuspensionController.cpp
    src/TelemetryFormatter.cpp
    src/TelemetryLogger.cpp
)


# ============================================================
# Host application
# ============================================================

add_executable(project1
    src/main.cpp
    ${PROJECT_SOURCES}
)

target_include_directories(project1 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


# ============================================================
# Tests
# ============================================================

enable_testing()

add_executable(unit_tests
    tests/test_main.cpp
    ${PROJECT_SOURCES}
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(
    NAME unit_tests
    COMMAND unit_tests
)

add_executable(iot_test
    tests/test_iot.cpp
)

target_include_directories(iot_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(
    NAME iot_test
    COMMAND iot_test
)


# ============================================================
# STM32F4 + FreeRTOS
# ============================================================

option(
    BUILD_STM32
    "Build STM32F4 FreeRTOS target"
    OFF
)

if(BUILD_STM32)

    message(STATUS "========================================")
    message(STATUS "Building STM32F4 FreeRTOS target")
    message(STATUS "========================================")


    # ========================================================
    # ARM tools
    # ========================================================

    find_program(ARM_GCC arm-none-eabi-gcc REQUIRED)
    find_program(ARM_GXX arm-none-eabi-g++ REQUIRED)
    find_program(ARM_OBJCOPY arm-none-eabi-objcopy REQUIRED)
    find_program(ARM_SIZE arm-none-eabi-size)


    message(STATUS "ARM GCC     : ${ARM_GCC}")
    message(STATUS "ARM G++     : ${ARM_GXX}")
    message(STATUS "ARM objcopy : ${ARM_OBJCOPY}")


    # ========================================================
    # CPU
    # ========================================================

    set(STM32_CPU_FLAGS
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
    )


    # ========================================================
    # Compiler flags
    # ========================================================

    set(STM32_COMMON_FLAGS
        ${STM32_CPU_FLAGS}
        -ffunction-sections
        -fdata-sections
        -Wall
        -Wextra
    )


    # ========================================================
    # FreeRTOS directories
    # ========================================================

    set(FREERTOS_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS
    )

    set(FREERTOS_PORT_DIR
        ${FREERTOS_DIR}/portable/GCC/ARM_CM4F
    )


    # ========================================================
    # STM32 application sources
    # ========================================================

    set(STM32_APPLICATION_SOURCES

        freertos_port/src/main.cpp

        src/CoilController.cpp
        src/CoilDriver.cpp
        src/ComfortStrategy.cpp
        src/DampingCalculator.cpp
        src/SensorReader.cpp
        src/SportStrategy.cpp
        src/SuspensionController.cpp
        src/TelemetryFormatter.cpp
        src/TelemetryLogger.cpp

        freertos_port/src/stm32f4xx_hw.c
        freertos_port/src/syscalls.c
    )


    # ========================================================
    # STM32 ELF
    # ========================================================

    add_executable(qemu_stm32.elf

        freertos_port/src/startup.s

        ${STM32_APPLICATION_SOURCES}

        # FreeRTOS kernel
        ${FREERTOS_DIR}/tasks.c
        ${FREERTOS_DIR}/queue.c
        ${FREERTOS_DIR}/list.c
        ${FREERTOS_DIR}/timers.c
        ${FREERTOS_DIR}/event_groups.c
        ${FREERTOS_DIR}/stream_buffer.c

        # FreeRTOS heap
        ${FREERTOS_DIR}/portable/MemMang/heap_4.c

        # FreeRTOS Cortex-M4F port
        ${FREERTOS_PORT_DIR}/port.c
    )


    # ========================================================
    # Include directories
    # ========================================================

    target_include_directories(qemu_stm32.elf PRIVATE

        ${CMAKE_CURRENT_SOURCE_DIR}/include

        ${CMAKE_CURRENT_SOURCE_DIR}/freertos_port/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/freertos_port/src

        ${FREERTOS_DIR}/include
        ${FREERTOS_PORT_DIR}
    )


    # ========================================================
    # Compiler options
    # ========================================================

    target_compile_options(qemu_stm32.elf PRIVATE

        ${STM32_COMMON_FLAGS}

        -fno-unwind-tables
        -fno-asynchronous-unwind-tables

        $<$<COMPILE_LANGUAGE:CXX>:
            -fno-exceptions
            -fno-rtti
        >
    )


    # ========================================================
    # Linker
    #
    # NO nosys.specs.
    #
    # syscalls.c provides:
    #   _sbrk
    #   _exit
    #   _write
    #   _close
    #   _read
    #   etc.
    # ========================================================

    target_link_options(qemu_stm32.elf PRIVATE

        ${STM32_CPU_FLAGS}

        -T${CMAKE_CURRENT_SOURCE_DIR}/freertos_port/src/stm32f4.ld

        -Wl,--gc-sections

        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/qemu_stm32.map
    )


    # ========================================================
    # Generate BIN + HEX
    # ========================================================

    add_custom_command(
        TARGET qemu_stm32.elf
        POST_BUILD

        COMMAND ${ARM_OBJCOPY}
            -O binary
            $<TARGET_FILE:qemu_stm32.elf>
            ${CMAKE_CURRENT_BINARY_DIR}/qemu_stm32.bin

        COMMAND ${ARM_OBJCOPY}
            -O ihex
            $<TARGET_FILE:qemu_stm32.elf>
            ${CMAKE_CURRENT_BINARY_DIR}/qemu_stm32.hex

        COMMENT "Generating qemu_stm32.bin and qemu_stm32.hex"
    )


    # ========================================================
    # Firmware size
    # ========================================================

    if(ARM_SIZE)

        add_custom_command(
            TARGET qemu_stm32.elf
            POST_BUILD

            COMMAND ${ARM_SIZE}
                $<TARGET_FILE:qemu_stm32.elf>

            COMMENT "STM32 firmware size"
        )

    endif()


    # ========================================================
    # Information
    # ========================================================

    message(STATUS "----------------------------------------")
    message(STATUS "STM32 target      : qemu_stm32.elf")
    message(STATUS "CPU               : Cortex-M4F")
    message(STATUS "FreeRTOS port     : ARM_CM4F")
    message(STATUS "Linker script     : freertos_port/src/stm32f4.ld")
    message(STATUS "Syscalls          : custom syscalls.c")
    message(STATUS "nosys.specs       : DISABLED")
    message(STATUS "----------------------------------------")

endif()
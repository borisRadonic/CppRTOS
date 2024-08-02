cmake_minimum_required(VERSION 3.20)

project(TestRtos C CXX ASM)


# Include toolchain settings
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/rtos-gcc.cmake" CACHE STRING "Toolchain file")

message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Real Time Platform")
    ###################### CONSTANTS ######################################
    set(PROJECT_NAME "TestRtos")
    set(PROJECT_TYPE_EXECUTABLE "exe")
    set(PROJECT_TYPE_STATIC_LIBRARY "static-lib")
    set(RUNTIME_LIBRARY_FLAGS "--specs=nano.specs")
    set(RUNTIME_LIBRARY_SYSCALLS_FLAGS "--specs=nosys.specs")

    # Platforms
    set(PLATFORM_STM32H7 "stm32h7")
    set(PLATFORM_ZYNQ "zynq7")

    set(HW_PLATFORM "stm32h7")
endif()

if(${HW_PLATFORM} STREQUAL ${PLATFORM_STM32H7})
    set(MFLOAT_ABI_FLAGS "-mfloat-abi=hard")
    # Define the path to the CMSIS and device headers
    set(CMSIS_PATH "Test/TestRtos/TestStm32h750/Drivers/CMSIS/Include")
    set(DEVICE_PATH "Test/TestRtos/TestStm32h750/Drivers/CMSIS/Device/ST/STM32H7xx/Include")
    set(HAL_PATH "Test/TestRtos/TestStm32h750/Drivers/STM32H7xx_HAL_Driver/Inc")
    set(TEST_INC_PATH "Test/TestRtos/TestStm32h750/Inc")
    set(TEST_RTOS_PATH, "Test/TestRtos")
    set(PORT_PATH "Ports/GCC/ARMCM7")
    set(MCPU_FLAGS "-mcpu=cortex-m7")
    set(MFPU_FLAGS "-mfpu=fpv5-sp-d16") # Single-precision FPU
    set(MFLOAT_ABI ${MFLOAT_ABI_HARDWARE})
    set(RUNTIME_LIBRARY ${RUNTIME_LIBRARY_REDUCED_C})
    set(RUNTIME_LIBRARY_SYSCALLS ${RUNTIME_LIBRARY_SYSCALLS_MINIMAL})
elseif(${HW_PLATFORM} STREQUAL ${PLATFORM_ZYNQ})
    # Add ZYNQ specific settings here
endif()

#######################################################################

if(${HW_PLATFORM} STREQUAL ${PLATFORM_STM32H7})
    message(STATUS "Platform: ${PLATFORM_STM32H7}")
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Test/TestRtos/TestStm32h750/STM32H750XBHX_FLASH.ld)
    set(PROJECT_SOURCES
        ${CMAKE_SOURCE_DIR}/Test/TestRtos/TestStm32h750/Startup/startup_stm32h750xbhx.s
        ${CMAKE_SOURCE_DIR}/Test/TestRtos/TestStm32h750/Sources/main.cpp
        ${CMAKE_SOURCE_DIR}/Test/TestRtos/TestStm32h750/Sources/syscalls.c
        ${CMAKE_SOURCE_DIR}/Test/TestRtos/TestStm32h750/Sources/sysmem.c
        ${CMAKE_SOURCE_DIR}/Test/TestRtos/TestStm32h750/Sources/system_stm32h7xx.c
        ${CMAKE_SOURCE_DIR}/Test/TestRtos/TestStm32h750/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c
        ${CMAKE_SOURCE_DIR}/Test/TestRtos/TestStm32h750/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c
        ${CMAKE_SOURCE_DIR}/Test/TestRtos/TestStm32h750/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c
        ${CMAKE_SOURCE_DIR}/Test/TestRtos/TestStm32h750/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c
        ${CMAKE_SOURCE_DIR}/Test/TestRtos/TestStm32h750/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c
        ${CMAKE_SOURCE_DIR}/Test/TestRtos/TestStm32h750/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c
        ${CMAKE_SOURCE_DIR}/${PORT_PATH}/Port.cpp
        ${CMAKE_SOURCE_DIR}/${PORT_PATH}/PendHandlers.S
        ${CMAKE_SOURCE_DIR}/${PORT_PATH}/PortHwAssembly.S
        ${CMAKE_SOURCE_DIR}/Source/Kernel.cpp
        ${CMAKE_SOURCE_DIR}/Source/Task.cpp
        ${CMAKE_SOURCE_DIR}/Source/Mutex.cpp
        ${CMAKE_SOURCE_DIR}/Source/Semaphore.cpp
        ${CMAKE_SOURCE_DIR}/Source/Timer.cpp
    )
elseif(${HW_PLATFORM} STREQUAL ${PLATFORM_ZYNQ})
    # Add ZYNQ specific sources here
endif()

set(PROJECT_INCLUDES
    ${CMAKE_SOURCE_DIR}/${TEST_RTOS_PATH}
    ${CMAKE_SOURCE_DIR}/Include
    ${CMAKE_SOURCE_DIR}/${TEST_INC_PATH}
    ${CMAKE_SOURCE_DIR}/${PORT_PATH}

    ${CMAKE_SOURCE_DIR}/${HAL_PATH}
    ${CMAKE_SOURCE_DIR}/${CMSIS_PATH}
    ${CMAKE_SOURCE_DIR}/${DEVICE_PATH}
)

message(STATUS "Project includes: ${PROJECT_INCLUDES}")

set(PROJECT_TYPE "exe")

# Add include directories
include_directories(${PROJECT_INCLUDES})

#######################################################################

################## PROJECT SETUP ######################################
enable_language(ASM)

if(${PROJECT_TYPE} STREQUAL ${PROJECT_TYPE_EXECUTABLE})
    # Define executable
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>)
elseif(${PROJECT_TYPE} STREQUAL ${PROJECT_TYPE_STATIC_LIBRARY})
    add_library(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE STM32H750xx) # Add the define here

# Define common flags as a list of separate items
#set(COMMON_FLAGS -mthumb -Wall -Werror)

# Define C and C++ compiler flags separately as lists of separate items
set(C_FLAGS -std=gnu11 ${MCPU_FLAGS} ${MFPU_FLAGS} ${MFLOAT_ABI_FLAGS} ${RUNTIME_LIBRARY_FLAGS})
set(CXX_FLAGS -std=c++17 ${MCPU_FLAGS} ${MFPU_FLAGS} ${MFLOAT_ABI_FLAGS} ${RUNTIME_LIBRARY_FLAGS})
set(ASM_FLAGS -x assembler-with-cpp ${MCPU_FLAGS} ${MFPU_FLAGS} ${MFLOAT_ABI_FLAGS} ${RUNTIME_LIBRARY_FLAGS})

# Apply the C and C++ flags separately using target_compile_options
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:${COMMON_FLAGS} ${C_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${COMMON_FLAGS} ${CXX_FLAGS}>
    $<$<COMPILE_LANGUAGE:ASM>:${COMMON_FLAGS} ${ASM_FLAGS}>
)

# Apply linker options
target_link_options(${PROJECT_NAME} PRIVATE
    -T${LINKER_SCRIPT}
    ${RUNTIME_LIBRARY_SYSCALLS_FLAGS}
    ${MCPU_FLAGS}
    ${MFPU_FLAGS}
    ${MFLOAT_ABI_FLAGS}
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--gc-sections
    -static
    -Wl,--start-group
    -lc
    -lm
    -Wl,--end-group
)

# Set the executable suffix
set(CMAKE_EXECUTABLE_SUFFIX ".elf")


# Add include directories
include_directories(${PROJECT_INCLUDES})

# Define common flags as a list of separate items
#set(COMMON_FLAGS "-mthumb -Wall -Werror")
set(CXX_FLAGS "-fno-threadsafe-statics -std=c++17 ${MCPU_FLAGS} ${MFPU_FLAGS} ${MFLOAT_ABI_FLAGS} ${RUNTIME_LIBRARY_FLAGS}")

# Construct include directories flags
set(INCLUDE_DIRS "")
foreach(INCLUDE_DIR ${PROJECT_INCLUDES})
    set(INCLUDE_DIRS "${INCLUDE_DIRS} -I${INCLUDE_DIR}")
endforeach()

# Ensure INCLUDE_DIRS is properly set
message(STATUS "Include directories: ${INCLUDE_DIRS}")

# Generate assembly for Port.cpp
set(PORT_CPP "${CMAKE_SOURCE_DIR}/${PORT_PATH}/Port.cpp")
set(PORT_ASM "${CMAKE_BINARY_DIR}/assembly/Port.s")

set(GEN_S_CXX_FLAGS ${MCPU_FLAGS} ${MFPU_FLAGS} ${MFLOAT_ABI_FLAGS} ${RUNTIME_LIBRARY_FLAGS})

#add_custom_command(
#    OUTPUT ${PORT_ASM}
#    COMMAND ${CMAKE_CXX_COMPILER} ${COMMON_FLAGS} ${GEN_S_CXX_FLAGS} ${INCLUDE_DIRS} -S -o ${PORT_ASM} ${PORT_CPP}
#    DEPENDS ${PORT_CPP}
#    COMMENT "Generating assembly for Port.cpp"
#    USES_TERMINAL
#    VERBATIM
#)

#add_custom_target(GeneratePortAssembly ALL DEPENDS ${PORT_ASM})

# Print the command for debugging
#message(STATUS "Custom command: ${CMAKE_CXX_COMPILER} ${CXX_FLAGS} ${COMMON_FLAGS} ${INCLUDE_DIRS} -S -o ${PORT_ASM} ${PORT_CPP}")
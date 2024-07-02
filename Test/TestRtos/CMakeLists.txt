
cmake_minimum_required(VERSION 3.20)
project(TestRtos C CXX ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###################### CONSTANTS ######################################
set(PROJECT_TYPE_EXECUTABLE "exe")
set(PROJECT_TYPE_STATIC_LIBRARY "static-lib")
set(MCPU_FLAGS "-mcpu=cortex-m7")
set(MFPU_FLAGS "-mfpu=fpv5-sp-d16") # Single-precision FPU
set(MFLOAT_ABI_FLAGS "-mfloat-abi=hard")
set(RUNTIME_LIBRARY_FLAGS "--specs=nano.specs")
set(RUNTIME_LIBRARY_SYSCALLS_FLAGS "--specs=nosys.specs")


# Define the path to the CMSIS and device headers
set(CMSIS_PATH "Drivers/CMSIS/Include")
set(DEVICE_PATH "Drivers/CMSIS/Device/ST/STM32H7xx/Include")
set(HAL_PATH "Drivers/STM32H7xx_HAL_Driver/Inc")

# Add include directories
include_directories(${CMSIS_PATH} ${DEVICE_PATH})

#######################################################################



set(PROJECT_SOURCES
    Startup/startup_stm32h750xbhx.s
    Sources/main.cpp
    Sources/syscalls.c
    Sources/sysmem.c
    Sources/system_stm32h7xx.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c
    ../../Source/Port.cpp
)

set(PROJECT_DEFINES
    # LIST COMPILER DEFINITIONS HERE
)

set(PROJECT_INCLUDES
    ../../Include
    Inc
    ${HAL_PATH}
    ${CMSIS_PATH}
    ${DEVICE_PATH}
)
#######################################################################

################## PROJECT SETUP ######################################
enable_language(ASM)

if(${PROJECT_TYPE} STREQUAL ${PROJECT_TYPE_EXECUTABLE})
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>)
elseif(${PROJECT_TYPE} STREQUAL ${PROJECT_TYPE_STATIC_LIBRARY})
    add_library(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()


target_compile_definitions(${PROJECT_NAME} PRIVATE STM32H750xx) # Add the define here

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDES})

# Define common flags as a list of separate items
set(COMMON_FLAGS -mthumb -Wall -Werror)

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
set(CMAKE_STATIC_LIBRARY_SUFFIX ".a")

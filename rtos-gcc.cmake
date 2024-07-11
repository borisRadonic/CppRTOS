# cubeide-gcc.cmake
set (CMAKE_SYSTEM_PROCESSOR "arm" CACHE STRING "")
set (CMAKE_SYSTEM_NAME "Generic" CACHE STRING "")

# Skip link step during toolchain validation.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(TOOLCHAIN_PREFIX   "arm-none-eabi-")
set(CMAKE_C_COMPILER   "${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++")
set(CMAKE_AR           "${TOOLCHAIN_PREFIX}ar")
set(CMAKE_LINKER       "${TOOLCHAIN_PREFIX}ld")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_PREFIX}objcopy")
set(CMAKE_RANLIB       "${TOOLCHAIN_PREFIX}ranlib")
set(CMAKE_SIZE         "${TOOLCHAIN_PREFIX}size")
set(CMAKE_STRIP        "${TOOLCHAIN_PREFIX}ld")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Define constants and variables
set(MCPU_CORTEX_M7 "-mcpu=cortex-m7")
set(MFPU_FPV4_SP_D16 "-mfpu=fpv4-sp-d16")
set(MFPU_FPV5_D16 "-mfpu=fpv5-d16")
set(MFLOAT_ABI_HARDWARE "-mfloat-abi=hard")
set(RUNTIME_LIBRARY_REDUCED_C "--specs=nano.specs")
set(RUNTIME_LIBRARY_SYSCALLS_MINIMAL "--specs=nosys.specs")

# Set default values
set(MCPU ${MCPU_CORTEX_M7})
set(MFPU ${MFPU_FPV4_D16})
set(MFLOAT_ABI ${MFLOAT_ABI_HARDWARE})
set(RUNTIME_LIBRARY ${RUNTIME_LIBRARY_REDUCED_C})
set(RUNTIME_LIBRARY_SYSCALLS ${RUNTIME_LIBRARY_SYSCALLS_MINIMAL})

# No optimization for Debug
set(CMAKE_C_FLAGS_DEBUG    "-g3 -O0 -DDEBUG" CACHE STRING "C debug flags")
set(CMAKE_CXX_FLAGS_DEBUG  "-fno-exceptions -fno-rtti -g3 -O0 -DDEBUG" CACHE STRING "CXX debug flags")
set(CMAKE_ASM_FLAGS_DEBUG  "-g3 -O0 -DDEBUG" CACHE STRING "ASM debug flags")

# Optimize on size for Release
set(CMAKE_C_FLAGS_RELEASE      "-Os -DNDEBUG" CACHE STRING "C release flags")
set(CMAKE_CXX_FLAGS_RELEASE    "-Os -DNDEBUG" CACHE STRING "CXX release flags")
set(CMAKE_ASM_FLAGS_RELEASE    "-Os -DNDEBUG" CACHE STRING "ASM release flags")

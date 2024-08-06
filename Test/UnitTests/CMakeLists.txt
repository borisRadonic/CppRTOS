# CMakeList.txt : CMake project for CMakeProject1, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

#set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/ut-windows.cmake" CACHE STRING "Toolchain file")

# Set compilers and system-specific settings at the top
project(CppRtosUT LANGUAGES C CXX ASM_MASM)



# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)


# Include the FetchContent module
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 17)


# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)


if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  message(STATUS "Configuring for Windows")
  # Enable MASM assembler for Visual Studio
  enable_language(ASM_MASM)

  # Use Visual Studio's native generator if possible
  if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
    set(CMAKE_GENERATOR "Visual Studio 17 2022")
  endif()
endif()


set(UT_SOURCES
    ${PROJECT_SOURCE_DIR}/Scheduling_Test.cpp
    ${PROJECT_SOURCE_DIR}/Mutex_Test.cpp
    ${PROJECT_SOURCE_DIR}/Semaphore_Test.cpp
    ${PROJECT_SOURCE_DIR}/Port.cpp
)


set(CODE_UNDER_TEST_SOURCES
    ${PROJECT_SOURCE_DIR}/../../Source/Kernel.cpp
    ${PROJECT_SOURCE_DIR}/../../Source/Mutex.cpp
    ${PROJECT_SOURCE_DIR}/../../Source/Semaphore.cpp
    ${PROJECT_SOURCE_DIR}/../../Source/Task.cpp
    ${PROJECT_SOURCE_DIR}/../../Source/Timer.cpp
)

message(STATUS "UT sources: ${UT_SOURCES}")
message(STATUS "Code under test: ${CODE_UNDER_TEST_SOURCES}")

set(INCLUDE_DIRS   
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/gtest
    ${PROJECT_SOURCE_DIR}/gtest/utils
    ${PROJECT_SOURCE_DIR}/gtest/mock_files
    ${PROJECT_SOURCE_DIR}/../../Include  
)

# Set stack size, relevant for Windows
if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:1048576")
endif()



add_executable              (CppRtosUT ${UT_SOURCES} ${CODE_UNDER_TEST_SOURCES})

target_include_directories  (CppRtosUT PRIVATE ${INCLUDE_DIRS})


target_link_libraries   (CppRtosUT PRIVATE GTest::gtest_main GTest::gmock GTest::gmock_main)


target_link_options         (CppRtosUT PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wl,-Map=UnitTests.map>)

enable_testing()


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CppRtosUT PROPERTY CXX_STANDARD 17)
endif()

add_test(NAME CppRtosUT COMMAND CppRtosUT)


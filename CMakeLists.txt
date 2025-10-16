cmake_minimum_required(VERSION 3.16)
project(MarketMakingOptimizer VERSION 1.0.0 LANGUAGES CXX)

# --------------------------
# C++ settings
# --------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /O1 /Ob1 /W3 /arch:AVX2")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2 /Oi /Ot /Oy /GL /DNDEBUG /arch:AVX2 /fp:fast")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O1 -Wall -Wextra -march=native")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -ffast-math")
endif()

# --------------------------
# vcpkg toolchain
# --------------------------
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR "You must set CMAKE_TOOLCHAIN_FILE to vcpkg's toolchain file!")
endif()

# --------------------------
# Dependencies via vcpkg
# --------------------------
find_package(OpenBLAS CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(libwebsockets CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
# Optimized implementation with pre-computed constants and batch processing

# --------------------------
# Source and header files
# --------------------------
set(SOURCES
    src/core/MarketData.cpp
    src/core/Quote.cpp
    src/core/Fill.cpp
    src/core/Position.cpp
    src/core/PnLTracker.cpp
    src/core/AvellanedaStoikov.cpp
    src/core/Logger.cpp
    src/core/LatencyBenchmark.cpp
    src/core/MarketSimulator.cpp
    src/core/DatabaseManager.cpp
)

set(HEADERS
    include/MarketData.h
    include/Quote.h
    include/Fill.h
    include/Position.h
    include/PnLTracker.h
    include/AvellanedaStoikov.h
    include/Logger.h
    include/LatencyBenchmark.h
    include/Symbol.h
    include/MarketSimulator.h
    include/DatabaseManager.h
)

# --------------------------
# Core library
# --------------------------
add_library(market_making_core STATIC ${SOURCES} ${HEADERS})

# Link dependencies
target_link_libraries(market_making_core
    PRIVATE
        OpenBLAS::OpenBLAS
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
)

# Include your own headers
target_include_directories(market_making_core
    PRIVATE
        include
)

# Windows sockets
if(WIN32)
    target_link_libraries(market_making_core PRIVATE ws2_32)
endif()

# --------------------------
# Example executables
# --------------------------
add_executable(example_main src/examples/main.cpp)
target_link_libraries(example_main PRIVATE market_making_core)

add_executable(simulation_test src/examples/simulation_test.cpp)
target_link_libraries(simulation_test PRIVATE market_making_core)

add_executable(simple_as_engine src/examples/simple_as_engine.cpp)
target_include_directories(simple_as_engine PRIVATE include)
target_link_libraries(simple_as_engine PRIVATE market_making_core websockets_shared)

# --------------------------
# Testing
# --------------------------
enable_testing()
add_subdirectory(tests)
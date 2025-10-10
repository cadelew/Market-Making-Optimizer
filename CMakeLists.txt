cmake_minimum_required(VERSION 3.16)
project(MarketMakingOptimizer VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find packages
find_package(xtensor REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(pybind11 REQUIRED)

# Include directories
include_directories(include)
include_directories(${xtensor_INCLUDE_DIRS})

# Source files
set(SOURCES
    src/core/MarketData.cpp
    src/core/Quote.cpp
    src/core/PnLTracker.cpp
    src/core/AvellanedaStoikov.cpp
    src/core/Logger.cpp
)

# Header files
set(HEADERS
    include/MarketData.h
    include/Quote.h
    include/PnLTracker.h
    include/AvellanedaStoikov.h
    include/Logger.h
)

# Create the main library
add_library(market_making_core STATIC ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(market_making_core 
    xtensor::xtensor
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

# Python bindings
pybind11_add_module(market_making_bindings 
    src/bindings/python_bindings.cpp
)

target_link_libraries(market_making_bindings 
    market_making_core
    pybind11::module
)

# Tests
enable_testing()
add_subdirectory(tests)

# Example executable
add_executable(example_main src/examples/main.cpp)
target_link_libraries(example_main market_making_core)

# Installation
install(TARGETS market_making_core market_making_bindings
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)



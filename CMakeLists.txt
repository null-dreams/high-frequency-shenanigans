cmake_minimum_required(VERSION 3.15)
project(LimitOrderBook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define the library
add_library(orderbook
    src/OrderBook.cpp
)

target_include_directories(orderbook PUBLIC include)

# Define the executable
add_executable(lob_runner main.cpp)
target_link_libraries(lob_runner PRIVATE orderbook)
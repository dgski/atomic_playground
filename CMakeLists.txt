cmake_minimum_required (VERSION 3.15)

project(cmake_test
  LANGUAGES CXX
  VERSION 1.0
)

set (CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -ld_classic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory(WaitFreeChannel)
add_subdirectory(WaitFreeQueue)
add_subdirectory(WaitFreeAsyncWorker)
add_subdirectory(WaitFreeChannelSingleUse)

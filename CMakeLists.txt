cmake_minimum_required(VERSION 3.0)
project(Carabiner)

include(~/git/link/AbletonLinkConfig.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(mongoose_HEADERS
  mongoose.h
)

set(mongoose_SOURCES
  mongoose.c
)

add_executable(Carabiner
  ${mongoose_HEADERS}
  ${mongoose_SOURCES}
  carabiner.cpp
)

target_link_libraries(Carabiner Ableton::Link)


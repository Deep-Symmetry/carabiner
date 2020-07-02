cmake_minimum_required(VERSION 3.0)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")

project(Carabiner)

set(Carabiner_VERSION "1.1.4")
configure_file(carabiner.h.in carabiner.h)
configure_file(Info.plist.in Info.plist)

include(link/AbletonLinkConfig.cmake)
add_subdirectory(gflags)

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

target_include_directories(Carabiner PUBLIC "${PROJECT_BINARY_DIR}")

if (APPLE)
   target_link_options(Carabiner PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${PROJECT_BINARY_DIR}/Info.plist)
endif()

target_link_libraries(Carabiner Ableton::Link gflags)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  target_link_libraries(Carabiner pthread)
endif()

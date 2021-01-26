cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
project(file-transfer-app)

set(CMAKE_BUILD_TYPE Debug)
#list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

LIST(APPEND SRV_SOURCES src/main_server.cpp src/main_common.cpp src/server.cpp src/file_operations.cpp)
LIST(APPEND CLI_SOURCES src/main_client.cpp src/main_common.cpp src/client.cpp)
     
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(ft_server ${SRV_SOURCES})
add_executable(ft_client ${CLI_SOURCES})

target_link_libraries(ft_server PRIVATE ${CONAN_LIBS})
target_link_libraries(ft_client PRIVATE ${CONAN_LIBS})

cmake_minimum_required(VERSION 3.12.3)
project(Client)

set(CMAKE_CXX_STANDARD 11)
find_package(Boost REQUIRED COMPONENTS system)
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost REQUIRED COMPONENTS system)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
# the call to include_directories is now useless:
# the Boost::system imported target used below
# embeds the include directories

project(Client C CXX)
add_executable(Client src/echoClient.cpp src/connectionHandler.cpp)
target_link_libraries(Client Boost::system)


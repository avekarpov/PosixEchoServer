cmake_minimum_required(VERSION 3.29)
project(PosixSocket)

set(CMAKE_CXX_STANDARD 20)

add_executable(
    PosixSocket
    main.cpp
    echo_server.cpp
    tools.cpp
)

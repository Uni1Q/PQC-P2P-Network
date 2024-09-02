cmake_minimum_required(VERSION 3.29)
project(untitled C)

set(CMAKE_C_STANDARD 11)

add_executable(untitled test/main.c
        netlib.h
        Networking/Nodes/Server.h
        Networking/Nodes/Server.c
        Networking.h
        Networking/Nodes/Client.c
        Networking/Nodes/Client.h)

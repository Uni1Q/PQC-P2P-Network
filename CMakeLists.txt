cmake_minimum_required(VERSION 3.29)
project(untitled C)

set(CMAKE_C_STANDARD 11)

add_executable(untitled test/main.c
        netlib.h
        Networking/Nodes/Server.h
        Networking/Nodes/Server.c
        Networking.h
        Networking/Nodes/Client.c
        Networking/Nodes/Client.h
        "Systems.h"
        Systems/ThreadPool.c
        Systems/ThreadPool.h
        DataStructures/Dictionary/Dictionary.c
        DataStructures/Dictionary/Dictionary.h
        DataStructures/Dictionary/Entry.c
        DataStructures/Dictionary/Entry.h
        "DataStructures/Trees/BinarySearchTree.c"
        "DataStructures/Trees/BinarySearchTree.h"
        DataStructures/Common/Node.c
        DataStructures/Common/Node.h
        DataStructures/Lists/LinkedList.c
        DataStructures/Lists/LinkedList.h
        Cryptography/Multiprecision/ArbitrarilyPreciseInteger.c
        Cryptography/Multiprecision/ArbitrarilyPreciseInteger.h
        DataStructures/Lists/Queue.c
        DataStructures/Lists/Queue.h
        Networking/Nodes/PeerToPeer.c
        Networking/Nodes/PeerToPeer.h
        Networking/Nodes/HTTPServer.c
        Networking/Nodes/HTTPServer.h
        Networking/Protocols/HTTPRequest.c
        Networking/Protocols/HTTPRequest.h
        Cryptography.h
        DataStructures.h
        Systems/Files.c
        Systems/Files.h
        Networking/Protocols/BlockHeaders.c
        Networking/Protocols/BlockHeaders.h)

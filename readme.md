# Dissertation project software application For Solent University by **Rokas Jodka**

To run the app in a Windows environment:

 1. Install Cygwin with the following extensions:
	- gcc-core
	- gdb
	- make
 2. Open Cygwin64 terminal
 3. cd [install directory]
 4. Type "make" in console and the application should compile
 5. ./bin/client_app to launch the client or ./bin/server_app to launch the routing server

If for any reason the application does not compile on Windows, follow the steps below on a linux environment:
1. Open terminal
2. Using the distributions package manager install the following
	- gcc
	- make
 3. cd [install directory]
 4. Type "make" in console and the application should compile
 5. ./bin/client_app to launch the client or ./bin/server_app to launch the routing server

### The application can work locally, although to connect to a remote peer, both peers no port forward on the port they are both using

To test the application locally you can open multiple instances of the Cygwin terminal and entering different ports. Adjust the IP in client/main.c the routing servers IP should be in the commented line besodes the line of code

```#define ROUTING_SERVER_IP "127.0.0.1"```

### The P2P Network with PQC encryption is sadly non-functional at this time, although the local KEM implementation is working, To run pqc_test:

1. Install and build openssl
2. Install and build liboqs
3. Move the built files to pqc_net directory
4. cd [pqc_net directory]
5. make
6. ./kyber_test

### The test directory contains test files and an older version of the network implementation. The test files can be compiled with make in the test directory and executables found in the bin directory
### The exe files cannot directly launched, they have to be built and launched through Cygwin64

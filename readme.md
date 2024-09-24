# Dissertation project software application For Solent University by **Rokas Jodka**

To run the app in a Windows environment:

 1. Install Cygwin with the following extentions:
	- gcc-core
	- gdb
	- make
	- openssl
 2. Open Cygwin64 terminal
 3. cd [install directory]
 4. Type "make" in console and the application should compile
 5. ./bin/client_app to launch the client or ./bin/server_app to launch the routing server

If for any issue the application does not compile on Windows, follow the steps below:
1. Open terminal
2. Using the distributions package manager install the following
	- gcc
	- make
	- openssl
 3. cd [install directory]
 4. Type "make" in console and the application should compile
 5. ./bin/client_app to launch the client or ./bin/server_app to launch the routing server

### The appplication can work locally, although to have it work over the internet you will need to port forward

### To test the application locally you can open multiple instances of the Cygwin terminal and entering different ports. Adjust the IP in client/main.c

```#define ROUTING_SERVER_IP "127.0.0.1"```

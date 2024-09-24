# Makefile for building client, routingServer, and testClient applications

# ===========================
#       Directory Setup
# ===========================

# Define the directories where the libraries are located
LIB_DIRS = ../DataStructures ../Systems ../Networking

# Define the target directory where the libraries will be copied
LIB_TARGET_DIR = ./libs

# ===========================
#       Library Definitions
# ===========================

# Define the library names with proper prefixes and paths
LIB_DATASTRUCTURES = $(LIB_TARGET_DIR)/libDataStructures.a
LIB_SYSTEM = $(LIB_TARGET_DIR)/libSystem.a
LIB_NETWORKING = $(LIB_TARGET_DIR)/libNetworking.a

# Aggregate all libraries for easy reference
LIBS = $(LIB_DATASTRUCTURES) $(LIB_SYSTEM) $(LIB_NETWORKING)

# ===========================
#     Source and Object Files
# ===========================

# Define the source files
CLIENT_SRC = client.c
SERVER_SRC = routingServer.c
TEST_CLIENT_SRC = testClient.c

# Define the corresponding object files
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)
TEST_CLIENT_OBJ = $(TEST_CLIENT_SRC:.c=.o)

# ===========================
#    Compiler and Flags
# ===========================

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall -Wextra -Wpedantic -Wshadow -Wpointer-arith -O3 -fomit-frame-pointer -pthread

# Linker flags
LDFLAGS = -L$(LIB_TARGET_DIR) -lDataStructures -lSystem -lNetworking -pthread

# Include directories
INCLUDES = -I.

# ===========================
#          Targets
# ===========================

# Phony targets to avoid conflicts with file names
.PHONY: all move-libs client routingServer testClient clean

# Default target: build everything
all: move-libs client routingServer testClient

# Target to create the libs directory and copy libraries
move-libs: $(LIBS)

# Rule to copy libDataStructures.a
$(LIB_TARGET_DIR)/libDataStructures.a: ../DataStructures/DataStructures.a | $(LIB_TARGET_DIR)
	cp ../DataStructures/DataStructures.a $@

# Rule to copy libSystem.a
$(LIB_TARGET_DIR)/libSystem.a: ../Systems/System.a | $(LIB_TARGET_DIR)
	cp ../Systems/System.a $@

# Rule to copy libNetworking.a
$(LIB_TARGET_DIR)/libNetworking.a: ../Networking/Networking.a | $(LIB_TARGET_DIR)
	cp ../Networking/Networking.a $@

# Rule to create the libs directory if it doesn't exist
$(LIB_TARGET_DIR):
	mkdir -p $(LIB_TARGET_DIR)

# Target to build the client executable
client: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o client $(CLIENT_OBJ) $(LDFLAGS)

# Target to build the routingServer executable
routingServer: $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o routingServer $(SERVER_OBJ) $(LDFLAGS)

# Target to build the testClient executable
testClient: $(TEST_CLIENT_OBJ)
	$(CC) $(CFLAGS) -o testClient $(TEST_CLIENT_OBJ) $(LDFLAGS)

# Target to clean build artifacts
clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) $(TEST_CLIENT_OBJ) client routingServer testClient
	# Note: Libraries in ./libs/ are preserved to avoid redundant copying

# ===========================
#   Compilation Rules
# ===========================

# Pattern rule to compile .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

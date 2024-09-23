# Define the directories
LIB_DIRS = ../DataStructures ../Systems ../Networking
LIB_TARGET_DIR = ./

# Define the library names with proper prefixes
LIB_DATASTRUCTURES = $(LIB_TARGET_DIR)libDataStructures.a
LIB_SYSTEM = $(LIB_TARGET_DIR)libSystem.a
LIB_NETWORKING = $(LIB_TARGET_DIR)libNetworking.a

# Define the source and object files
CLIENT_SRC = client.c
SERVER_SRC = routingServer.c

CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)

# Compiler and linker flags
CC = gcc
CFLAGS = -g -Wall -Wextra -Wpedantic -Wshadow -Wpointer-arith -O3 -fomit-frame-pointer -pthread
LDFLAGS = -L$(LIB_TARGET_DIR) -lDataStructures -lSystem -lNetworking -pthread

# Targets
all: move-libs client routingServer

move-libs:
	@echo "Moving and renaming libraries..."
	cp ../DataStructures/DataStructures.a $(LIB_DATASTRUCTURES)
	cp ../Systems/System.a $(LIB_SYSTEM)
	cp ../Networking/Networking.a $(LIB_NETWORKING)

client: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o client $(CLIENT_OBJ) $(LDFLAGS)

routingServer: $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o routingServer $(SERVER_OBJ) $(LDFLAGS)

clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) client routingServer
	rm -f *.a

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Makefile for PQC-P2P-Network Project

# Compiler
CC = gcc

# Directories
CLIENT_DIR = client
SERVER_DIR = server
COMMON_DIR = common
BIN_DIR = bin

# Compiler Flags
CFLAGS = -Wall -Wextra -Werror -pthread -g \
         -I$(CLIENT_DIR) \
         -I$(SERVER_DIR) \
         -I$(COMMON_DIR)

# Linker Flags
LDFLAGS = -pthread

# Source Files
CLIENT_SRCS = $(wildcard $(CLIENT_DIR)/*.c)
SERVER_SRCS = $(wildcard $(SERVER_DIR)/*.c)
COMMON_SRCS = $(wildcard $(COMMON_DIR)/*.c)

# Object Files
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
COMMON_OBJS = $(COMMON_SRCS:.c=.o)

# Executables with Unique Names
CLIENT_EXEC = client_app
SERVER_EXEC = server_app

# Default Target
all: $(BIN_DIR) $(CLIENT_EXEC) $(SERVER_EXEC)

# Create bin directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Client Build Rules
$(CLIENT_EXEC): $(CLIENT_OBJS) $(COMMON_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAGS)

# Server Build Rules
$(SERVER_EXEC): $(SERVER_OBJS) $(COMMON_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $^ $(LDFLAGS)

# Compilation Rules
$(CLIENT_DIR)/%.o: $(CLIENT_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_DIR)/%.o: $(SERVER_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean Target
clean:
	rm -f $(CLIENT_DIR)/*.o $(SERVER_DIR)/*.o $(COMMON_DIR)/*.o $(BIN_DIR)/$(CLIENT_EXEC) $(BIN_DIR)/$(SERVER_EXEC)

# Phony Targets
.PHONY: all clean

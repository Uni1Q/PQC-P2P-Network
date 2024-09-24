# oldMakefile

CC = gcc
CFLAGS = -Wall -Wextra -Werror -pthread -g -I./common -I./client -I./server
SRC_DIR = .
COMMON_DIR = common
CLIENT_DIR = client
SERVER_DIR = server
OBJ_DIR = obj
BIN_DIR = bin

# Define source files
COMMON_SOURCES = $(wildcard $(COMMON_DIR)/*.c)
CLIENT_SOURCES = $(wildcard $(CLIENT_DIR)/*.c)
SERVER_SOURCES = $(wildcard $(SERVER_DIR)/*.c)

# Define object files
COMMON_OBJECTS = $(COMMON_SOURCES:$(COMMON_DIR)/%.c=$(OBJ_DIR)/common/%.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:$(CLIENT_DIR)/%.c=$(OBJ_DIR)/client/%.o)
SERVER_OBJECTS = $(SERVER_SOURCES:$(SERVER_DIR)/%.c=$(OBJ_DIR)/server/%.o)

# Define targets
CLIENT_TARGET = $(BIN_DIR)/client_app
SERVER_TARGET = $(BIN_DIR)/server_app

# Default target
all: $(CLIENT_TARGET) $(SERVER_TARGET)

# Client executable
$(CLIENT_TARGET): $(CLIENT_OBJECTS) $(COMMON_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ -pthread

# Server executable
$(SERVER_TARGET): $(SERVER_OBJECTS) $(COMMON_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ -pthread

# Pattern rules
$(OBJ_DIR)/common/%.o: $(COMMON_DIR)/%.c
	@mkdir -p $(OBJ_DIR)/common
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/client/%.o: $(CLIENT_DIR)/%.c
	@mkdir -p $(OBJ_DIR)/client
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/server/%.o: $(SERVER_DIR)/%.c
	@mkdir -p $(OBJ_DIR)/server
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*

.PHONY: all clean

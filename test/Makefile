# test/Makefile

CC = gcc
CFLAGS = -Wall -Wextra -Werror -pthread -g -I../common
OBJ_DIR = obj
BIN_DIR = bin

COMMON_SOURCES = ../common/network.c ../common/utils.c
COMMON_OBJECTS = $(patsubst ../common/%.c,$(OBJ_DIR)/%.o,$(COMMON_SOURCES))

TEST_SOURCES = static_peer.c test_client.c get_peer_list.c
TEST_OBJECTS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_SOURCES))

TARGET_STATIC_PEER = $(BIN_DIR)/static_peer
TARGET_TEST_CLIENT = $(BIN_DIR)/test_client
TARGET_GET_PEER_LIST = $(BIN_DIR)/get_peer_list

all: $(TARGET_STATIC_PEER) $(TARGET_TEST_CLIENT) $(TARGET_GET_PEER_LIST)

$(TARGET_STATIC_PEER): $(OBJ_DIR)/static_peer.o $(COMMON_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ -pthread

$(TARGET_TEST_CLIENT): $(OBJ_DIR)/test_client.o $(COMMON_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ -pthread

$(TARGET_GET_PEER_LIST): $(OBJ_DIR)/get_peer_list.o $(COMMON_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ -pthread

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: ../common/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*

.PHONY: all clean

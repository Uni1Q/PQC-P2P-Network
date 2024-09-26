CC = gcc
CFLAGS = -std=c99 -I/cygdrive/c/Users/rokas/Desktop/PQC-P2P-Network/liboqs/include
LDFLAGS = -L/cygdrive/c/Users/rokas/Desktop/PQC-P2P-Network/liboqs/build/lib -loqs
TARGET = kyber_speed_test
SRC = kyber_test.c

all:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

TARGET = mysh

CC = gcc

SRC_DIR = src
INC_DIR = include

SRCS = $(SRC_DIR)/main.c \
	   $(SRC_DIR)/utils.c
	
CFLAGS = -Wall -I$(INC_DIR)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)
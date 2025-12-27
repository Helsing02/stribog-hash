CC = gcc
CFLAGS = -std=c11 -O2 -I../src -I. -D_GNU_SOURCE
LDFLAGS = -lm -lrt -pthread

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
RESULTS_DIR = results

STRIBOG_SRC = ../../src/hash/stribog.c
STRIBOG_OBJ = $(OBJ_DIR)/stribog.o

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS)) $(STRIBOG_OBJ)

TARGET = $(BIN_DIR)/stribog_benchmark

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR) $(RESULTS_DIR)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(STRIBOG_OBJ): $(STRIBOG_SRC)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

quick: $(TARGET)
	./$(TARGET) -s 1024 -S 16384 -i 10

full: $(TARGET)
	./$(TARGET) -s 1024 -S 1048576 -i 100 -o $(RESULTS_DIR)/full.csv

pro: $(TARGET)
	sudo ./$(TARGET) -s 1024 -S 1048576 -i 1000 -c 0 -r -o $(RESULTS_DIR)/pro.csv

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(RESULTS_DIR)

.PHONY: all clean quick full pro

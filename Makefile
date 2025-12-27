PROJECT_NAME = stribog

# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -Werror -march=native -std=c11 -pedantic -I./src

# Папки
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests
UNIT_DIR = $(TEST_DIR)/unit
COMP_DIR = $(TEST_DIR)/comparison
BENCH_DIR = $(TEST_DIR)/benchmark

# Исходные файлы
SRCS = $(shell find $(SRC_DIR) -name '*.c')
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Тесты
TEST_NAME = test_gost_examples
UNIT_SRC = $(UNIT_DIR)/$(TEST_NAME).c
UNIT_OBJ = $(OBJ_DIR)/$(TEST_NAME).o
UNIT_TARGET = $(BIN_DIR)/$(TEST_NAME)

# Цели
TARGET = $(BIN_DIR)/$(PROJECT_NAME)

# Правила
all: clean $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(OBJS) -o $@

$(UNIT_TARGET): $(filter-out $(OBJ_DIR)/main.o, $(OBJS)) $(UNIT_OBJ)
	@mkdir -p $(@D)
	$(CC) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/$(TEST_NAME).o: $(UNIT_SRC)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

compare:
	@echo "=== Сборка и запуск сравнения реализаций ==="
	@$(MAKE) -C $(COMP_DIR) clean all run_compare

benchmark:
	@echo "=== Сборка и запуск тестов производительности ==="
	@$(MAKE) -C $(BENCH_DIR) clean all full

clean:
	@$(MAKE) -C $(COMP_DIR) clean 2>/dev/null || true
	@$(MAKE) -C $(BENCH_DIR) clean 2>/dev/null || true
	rm -rf $(OBJ_DIR) $(BIN_DIR)

test: $(UNIT_TARGET)
	@echo "=== Запуск модульных тестов ==="
	@./$(UNIT_TARGET)

.PHONY: all debug release clean test compare benchmark

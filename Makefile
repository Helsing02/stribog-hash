PROJECT_NAME = stribog

# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -pedantic -I./src
# Флаги для отладки
DEBUG_FLAGS = -g -O0 -DDEBUG -DDEBUG_TRANSFORM
# Флаги для релиза
RELEASE_FLAGS = -O2 -DNDEBUG


# Папки
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests
UNIT_DIR = $(TEST_DIR)/unit
COMP_DIR = $(TEST_DIR)/comparsion

HASH_DIR = $(SRC_DIR)/hash
CLI_DIR = $(SRC_DIR)/cli
IO_DIR = $(SRC_DIR)/io

# Исходные файлы (автоматически находим все .c файлы)
SRCS = $(shell find $(SRC_DIR) -name '*.c')

# Объектные файлы (заменяем папку src на obj и расширение .c на .o)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Юнит тесты
UNIT_SRC = $(shell find $(UNIT_DIR) -name '*.c')
UNIT_OBJ = $(OBJ_DIR)/$(TEST_NAME).o
UNIT_TARGET = $(BIN_DIR)/stribog_test

# Имя исполняемого файла
TARGET = $(BIN_DIR)/$(PROJECT_NAME)

# Цель по умолчанию — сборка в режиме отладки
all: debug

# Цели debug/release
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

release: CFLAGS += $(RELEASE_FLAGS)
release: clean $(TARGET)

compare:
	@$(MAKE) -C $(COMP_DIR) all
	@echo "Running comparsion..."
	@$(MAKE) -C $(COMP_DIR) run_compare

# Сборка исполняемого файла
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@


# Сборка тестового исполняемого файла
$(UNIT_TARGET): $(filter-out $(OBJ_DIR)/main.o, $(OBJS)) $(UNIT_OBJ) | $(BIN_DIR)
	$(CC) $^ -o $@

# Сборка объектных файлов
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@) # Создаем папку для объектного файла, если ее нет
	$(CC) $(CFLAGS) -c $< -o $@

# Сборка тестового объектного файла
$(UNIT_OBJ): $(UNIT_SRC) | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

# Создание папок для объектных файлов и бинарника
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Очистка
compare-clean:
	@$(MAKE) -C $(COMP_DIR) clean

clean: compare-clean
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Пересборка
rebuild: clean all

# Цель для запуска тестов
unit: $(UNIT_TARGET)
	@echo "Running Stribog unit tests..."
	@./$(UNIT_TARGET)

# Указываем, что эти цели не являются реальными файлами
.PHONY: all clean rebuild unit compare compare-clean

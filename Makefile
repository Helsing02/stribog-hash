# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -pedantic -I./src
# Флаги для отладки
DEBUG_FLAGS = -g -O0 -DDEBUG -DDEBUG_TRANSFORM
# Флаги для релиза
RELEASE_FLAGS = -O2 -DNDEBUG

# Цель по умолчанию - сборка в режиме отладки
CFLAGS += $(DEBUG_FLAGS)

# Папки
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
HASH_DIR = $(SRC_DIR)/hash
CLI_DIR = $(SRC_DIR)/cli
IO_DIR = $(SRC_DIR)/io

# Исходные файлы (автоматически находим все .c файлы)
SRCS = $(wildcard $(SRC_DIR)/*.c) \
       $(wildcard $(HASH_DIR)/*.c) \
       $(wildcard $(CLI_DIR)/*.c) \
       $(wildcard $(IO_DIR)/*.c)

# Объектные файлы (заменяем папку src на obj и расширение .c на .o)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Имя исполняемого файла
TARGET = $(BIN_DIR)/stribog

# Основная цель
all: $(TARGET)

# Сборка исполняемого файла
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@

# Сборка объектных файлов
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@) # Создаем папку для объектного файла, если ее нет
	$(CC) $(CFLAGS) -c $< -o $@

# Создание папок для объектных файлов и бинарника
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Очистка
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Пересборка
rebuild: clean all

# Цель для запуска тестов
test: $(TARGET)
	@echo "Testing Stribog implementation..."
	@echo -n "" | ./$(TARGET) | xxd -p
	@echo -n "hello" | ./$(TARGET) | xxd -p
	@echo "Test completed."

# Указываем, что эти цели не являются реальными файдами
.PHONY: all clean rebuild test

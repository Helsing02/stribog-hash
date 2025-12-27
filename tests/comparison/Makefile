CC = gcc
CFLAGS = -std=c11 -fPIC -I../src -I.
LDFLAGS = -lm -ldl

# Директории
LIB_DIR = libs
SRC_DIR = src
ADAP_DIR = $(SRC_DIR)/adapters
BIN_DIR = bin
TD_DIR = test_data

# Реализации
IMPLS = my_impl other_impl_1 other_impl_2 other_impl_3
LIBS = $(addprefix $(LIB_DIR)/lib, $(addsuffix .so, $(IMPLS)))

# Исходные файлы
MY_IMPL_SRCS = ../../src/hash/stribog.c $(ADAP_DIR)/my_impl_adapter.c
OTHER_IMPL_1_SRCS = other_impl_1/C/standard/stribog.c $(ADAP_DIR)/other_impl_1_adapter.c
OTHER_IMPL_2_SRCS = other_impl_2/gost_3411_2012_calc.c $(ADAP_DIR)/other_impl_2_adapter.c
OTHER_IMPL_3_SRCS = $(ADAP_DIR)/other_impl_3_adapter.c 	# Header-only реализация

TARGETS = $(BIN_DIR)/compare $(BIN_DIR)/random_generator $(LIBS)

all: $(TARGETS)

# Создание директорий
$(LIB_DIR) $(OBJ_DIR) $(BIN_DIR):
	@mkdir -p $@

# ЯВНЫЕ ПРАВИЛА для каждой библиотеки
$(LIB_DIR)/libmy_impl.so: $(MY_IMPL_SRCS) | $(LIB_DIR)
	$(CC) $(CFLAGS) -shared -o $@ $(MY_IMPL_SRCS)

$(LIB_DIR)/libother_impl_1.so: $(OTHER_IMPL_1_SRCS) | $(LIB_DIR)
	$(CC) $(CFLAGS) -shared -o $@ $(OTHER_IMPL_1_SRCS)

$(LIB_DIR)/libother_impl_2.so: $(OTHER_IMPL_2_SRCS) | $(LIB_DIR)
	$(CC) $(CFLAGS) -shared -o $@ $(OTHER_IMPL_2_SRCS)

$(LIB_DIR)/libother_impl_3.so: $(OTHER_IMPL_3_SRCS) | $(LIB_DIR)
	$(CC) $(CFLAGS) -shared -o $@ $(OTHER_IMPL_3_SRCS)

# Утилиты
$(BIN_DIR)/compare: $(SRC_DIR)/compare.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Главный компаратор
$(BIN_DIR)/compare: $(COMPARE_OBJS) | $(LIB_DIR) $(BIN_DIR)
	$(CC) -o $@ $(COMPARE_OBJS) $(LDFLAGS)

$(BIN_DIR)/random_generator: $(GEN_OBJS) | $(BIN_DIR)
	$(CC) -o $@ $(GEN_OBJS) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGETS)
	rm -rf $(LIB_DIR) $(OBJ_DIR) $(BIN_DIR)

run_compare: all
	@echo "=== Запуск сравнения реализаций ==="
	@cd $(BIN_DIR) && ./compare

clean:
	rm -rf $(LIB_DIR) $(OBJ_DIR) $(BIN_DIR) $(TD_DIR)/random_*.bin

.PHONY: all clean run_compare

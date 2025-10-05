CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -fPIC -I../src -I.
LDFLAGS = -lm -ldl

# Директории
LIB_DIR = libs
SRC_DIR = src
ADAP_DIR = $(SRC_DIR)/adapters
OBJ_DIR = obj
BIN_DIR = bin


# Реализации (каждая соберётся в свою .so)
IMPLS = my_impl other_impl_1 other_impl_2 other_impl_3

# Автоматическая генерация путей
LIBS = $(addprefix $(LIB_DIR)/lib, $(addsuffix .so, $(IMPLS)))

TARGETS = $(BIN_DIR)/compare $(BIN_DIR)/random_generator $(LIBS)

# Исходники
MY_IMPL_SRCS = ../../src/hash/stribog.c $(ADAP_DIR)/my_impl_adapter.c
OTHER_IMPL_1_SRCS = other_impl_1/C/standard/stribog.c $(ADAP_DIR)/other_impl_1_adapter.c
OTHER_IMPL_2_SRCS = other_impl_2/gost_3411_2012_calc.c $(ADAP_DIR)/other_impl_2_adapter.c
OTHER_IMPL_3_SRCS = $(ADAP_DIR)/other_impl_3_adapter.c

COMPARE_SRCS = $(SRC_DIR)/compare.c
COMPARE_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(COMPARE_SRCS))

GEN_SRCS = $(SRC_DIR)/random_generator.c
GEN_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(GEN_SRCS))

all: $(TARGETS)

# Создаем директории
$(LIB_DIR):
	@mkdir -p $@

$(OBJ_DIR):
	@mkdir -p $@

$(BIN_DIR):
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
	@cd $(BIN_DIR) && ./compare

.PHONY: all clean run_compare

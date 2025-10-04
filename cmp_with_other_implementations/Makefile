CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -fPIC -I../src -I.
LDFLAGS = -lm -ldl

# Директории
LIB_DIR = libs
ADAP_DIR = adapters
GEN_DIR = generator

# Реализации (каждая соберётся в свою .so)
IMPLS = my_impl other_impl_1 other_impl_2 other_impl_3

# Автоматическая генерация путей
LIBS = $(addprefix $(LIB_DIR)/lib, $(addsuffix .so, $(IMPLS)))

TARGETS = compare $(GEN_DIR)/random_generator $(LIBS)

# Исходники
MY_IMPL_SRCS = ../src/hash/stribog.c $(ADAP_DIR)/my_impl_adapter.c
OTHER_IMPL_1_SRCS = other_impl_1/C/standard/stribog.c $(ADAP_DIR)/other_impl_1_adapter.c
OTHER_IMPL_2_SRCS = other_impl_2/gost_3411_2012_calc.c $(ADAP_DIR)/other_impl_2_adapter.c
OTHER_IMPL_3_SRCS = $(ADAP_DIR)/other_impl_3_adapter.c

COMPARE_SRCS = compare.c
COMPARE_OBJS = $(COMPARE_SRCS:.c=.o)

GEN_SRCS = $(GEN_DIR)/random_generator.c
GEN_OBJS = $(GEN_SRCS:.c=.o)

all: $(TARGETS)

$(LIB_DIR):
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
compare: $(COMPARE_OBJS) | $(LIB_DIR)
	$(CC) -o $@ $(COMPARE_OBJS) $(LDFLAGS)

random_generator: $(GEN_OBJS)
	$(CC) -o $(GEN_DIR)/$@ $< $(LDFLAGS)

%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGETS) $(COMPARE_OBJS) $(GEN_OBJS)
	rm -rf test_data/
	rm -rf $(LIB_DIR)

test: all
	./compare

.PHONY: all clean test
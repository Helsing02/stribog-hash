#include "stribog.h"
#include "stribog_const.h"
#include <string.h> // Для memcpy, memset

// Преобразование X (побитовая сумма по модулю 2 векторов размерностью 512)
static void stribog_X_transform(const uint8_t *a, const uint8_t *b, uint8_t *result) {
    /*
    Функция побитового суммирования по модулю 2 двух массивов.
    Безразлична к порядку байтов в массивах.
    Для ускорения кастит массивы к ``uint64_t``, позволяя обработать 8 байтов за одну итерацию.
    */
    for (int i = 0; i < STRIBOG_BLOCK_SIZE / 8; i++) {
        ((uint64_t *)result)[i] = ((uint64_t *)a)[i] ^ ((uint64_t *)b)[i];
    }
}

// Преобразование S (подстановка байтов по S-блоку)
static void stribog_S_transform(uint8_t *block) {
    /*
    Функция замены байтов по таблице.
    Безразлична к порядку байтов в массиве,
    Так как обрабатывает каждый байт отдельно и независимо от его положения в массиве.
    */ 
    for (int i = 0; i < STRIBOG_BLOCK_SIZE; i++) {
        block[i] = S_BOX[block[i]];
    }
}

// Преобразование P (перестановка байтов)
static void stribog_P_transform(uint8_t *block) {
    /*
    Функция перестановки байтов.
    Так как порядок перестановки имеет строгий математический характер
    И для каждого индекса элемента можно вычислить его новую позицию используя лишь одну формулу,
    Не использует таблиц перестановки.
    Безразлична к порядку байтов в массиве благодаря структуре перестановки.
    */
    uint8_t temp[STRIBOG_BLOCK_SIZE];
    
    // Сохраняем блок во временный массив
    memcpy(temp, block, STRIBOG_BLOCK_SIZE);
    
    for (int i = 0; i < STRIBOG_BLOCK_SIZE; i++) {
        block[i] = temp[(i * 8 + i / 8) % STRIBOG_BLOCK_SIZE];
    }
}

// Преобразвание l (умножение на матрицу)
static void multiply_by_l_matrix(uint8_t *data) {
    /*
    Функция умножения справа на матрицу, реализующая преобразование ``l``.
    Принимает массив длиной 8 байтов (64 бита).
    Ожидает по нулевому индексу младший байт из всей последовательности (с наименьшими индексами).
    e.g. data[0] = 7,6,5,4,3,2,1,0 data[7] = 63,62,61,60,59,58,57,56.
    При касте такого массива в одно число размером 64 бита (благодаря little endian хранению данных в памяти)
    получаем последовательный порядок бит *(uint64_t *)data = 63,62,61,...,2,1,0.
    */
    uint64_t result = 0ULL;

    // Проходим по всем битам
    for (int j = 0; j < 64; j++) {
        if (*(uint64_t *)data & (1ULL << j)) {
            // Cоответствующая строка матрицы участвует в суммировании
            result ^= L_MATRIX[63 - j];
        }
    }
    // Результат операции размещается в памяти аналогичным способом (младший байт имеет младший индекс)
    memcpy(data, &result, 8);
}

// Преобразование L (линейное преобразование)
static void stribog_L_transform(uint8_t *block) {
    /*
    Функция реализует преобразование ``L`` из ГОСТа.
    Ожидает в массиве порядок байтов от младшего к старшему (байт с нулевым индексом содержит биты с 7 по 0).
    */
    // По своей сути восемь умножений на матрицу восьми частей преобразуемого блока
    for (int i = 0; i < 8; i++) {
        multiply_by_l_matrix(&block[i * 8]);
    }
}

static void key_gen(uint8_t *K, uint8_t i) {
    /*
    Функция генерации очередного K на основе предыдущего.
    Ожидает в массиве порядок байтов от младшего к старшему (байт с нулевым индексом содержит биты с 7 по 0).
    */
    stribog_X_transform(K, C[i], K);
    stribog_S_transform(K);
    stribog_P_transform(K);
    stribog_L_transform(K);
}

// Функция E 
static void stribog_E_transform(uint8_t *K, const uint8_t *m, uint8_t *result) {
    /*
    Функция реализует преобразование ``E`` ГОСТа.
    Ожидает все массивы с порядком байтов от младшего к старшему (байт с нулевым индексом содержит биты с 7 по 0).
    Возвращает массив с аналогичным порядоком байтов.
    */
    uint8_t state[STRIBOG_BLOCK_SIZE];

    // Инициализация состояния и ключа
    memcpy(state, m, STRIBOG_BLOCK_SIZE);

    for (int round = 0; round < 12; round++) {
        // Применяем преобразования X, S, P, L
        stribog_X_transform(state, K, state);
        stribog_S_transform(state);
        stribog_P_transform(state);
        stribog_L_transform(state);

        // Генерируем новый ключ для следующего раунда
        key_gen(K, round);
    }
    // XOR ключа с состоянием
    stribog_X_transform(state, K, result);
}

// Функция сжатия g
static void stribog_g_transform(uint8_t *N, uint8_t *h, const uint8_t *m, uint8_t *result) {
    /*
    Функция реализует преобразование ``g`` ГОСТа.
    Ожидает все массивы с порядком байтов от младшего к старшему (байт с нулевым индексом содержит биты с 7 по 0).
    Возвращает массив с аналогичным порядоком байтов.
    */
    uint8_t key[STRIBOG_BLOCK_SIZE];
    uint8_t e_res[STRIBOG_BLOCK_SIZE];

    // Применяем преобразования X, S, P, L
    stribog_X_transform(N, h, key);
    stribog_S_transform(key);
    stribog_P_transform(key);
    stribog_L_transform(key);

    stribog_E_transform(key, m, e_res);

    stribog_X_transform(e_res, h, e_res);
    stribog_X_transform(e_res, m, result);
}

// Сложение двух 512-битных чисел по модулю 2^512
static void stribog_add_array_modulo_512(uint8_t *a, const uint8_t *b) {
    /*
    Функция сложения (по модулю 2^512) двух 512 битных чисел, представленных массивами байтов.
    Ожидает все массивы с порядком байтов от младшего к старшему (байт с нулевым индексом содержит биты с 7 по 0).
    Возвращает массив с аналогичным порядоком байтов.
    */
    uint16_t sum = 0;
    
    // Начинаем сумму с младших байтов (с меньшим индексом)
    for (int i = 0; i < STRIBOG_BLOCK_SIZE; i++) {
        sum = a[i] + b[i] + (sum >> 8);
        a[i] = sum;
    }
}

static void stribog_add_number_modulo_512(uint8_t *a, uint64_t b) {
    /*
    Функция сложения (по модулю 2^512) двух чисел, одно из которых представлено массивом, а второе 64-битным числом.
    Ожидает массив с порядком байтов от младшего к старшему (байт с нулевым индексом содержит биты с 7 по 0).
    Возвращает массив с аналогичным порядоком байтов.
    */
    ((uint64_t *)a)[0] += b;
    // Если произошло переполнение
    if (((uint64_t *)a)[0] < b) {
        for (int i = 1; i < 8; i++) {
            ((uint64_t *)a)[i] += 1;
            // Если не произошло переполнения
            if (a[i] != 0) {
                break;
            }
        }   
    }
}

void stribog_init(stribog_ctx_t *ctx, uint16_t hash_size) {
    /*
    Функция инициализации контекста хеширования. 
    Обнуляет структуру контекста, записывает и выставляет переменные,
    указанные в 1 этапе алгоритма из ГОСТа.
    */
    // Обнуляем структуру контекста
    memset(ctx, 0, sizeof(stribog_ctx_t));

    // Устанавливаем размер хеша
    ctx->hash_size = hash_size;

    // Инициализируем начальное значение h.
    // Для Stribog-512: h = 0x00...00 (инициализация нулями уже выполнена memset'ом)
    // Для Stribog-256: h = 0x010101...01 
    if (hash_size == 256) {
        memset(ctx->h, 0x01, STRIBOG_BLOCK_SIZE);
    }
}

void stribog_process_block(stribog_ctx_t *ctx, const uint8_t *m) {
    /*
    Вспомогательная функция обработки блока.
    Выполняет основные преобразования 2 этапа ГОСТа.
    */
    // Вызываем преобразование gN(h, m)
    stribog_g_transform(ctx->N, ctx->h, m, ctx->h);
    // Прибавляем 512 к N по модулю 2^512
    stribog_add_number_modulo_512(ctx->N, (uint64_t)512);
    // Прибавляем числовое представление m к Сигме
    stribog_add_array_modulo_512(ctx->Sigma, m);
}

void stribog_update(stribog_ctx_t *ctx, const uint8_t *data, size_t len){
    /* 
    Принимает конфигурацию хеша, массив байтов, и размер массива.
    Важно заметить, что по нулевому индексу массива должен лежать первый байт блока сообщения,
    несмотря на то, что в ГОСТе все 64 байтовые массивы представлены в обратном порядке байтов.
    */
    // Увеличиваем счетчик бит исходного сообщения на длину очередного блока в битах
    ctx->total_bits += (uint64_t)len * 8;

    // Обрабатываем данные по блокам
    while (len > 0) {
        // Вычисляем свободное место в буфере
        size_t to_copy = STRIBOG_BLOCK_SIZE - ctx->buffer_size;
        if (to_copy > len) {
            // Если свободного места оказалось больше чем размер переданной порции данных
            // То вся порция будет скопирована в буфер
            to_copy = len;
        }

        // Копирование в буфер
        memcpy(ctx->buffer + ctx->buffer_size, data, to_copy);
        // Обновляем переменную заполненности буфера
        ctx->buffer_size += to_copy;
        // Смещаем указатель на данные к первому байту из непрочитанных в буфер
        data += to_copy;
        // Уменьшаем оставшийся размер порции данных на количество считанных в буфер байтов
        len -= to_copy;

        // Если буфер контекста хеширования заполнился, можно провести итерацию хеширования
        if (ctx->buffer_size == STRIBOG_BLOCK_SIZE) {
            stribog_process_block(ctx, ctx->buffer);
            // Обнуляем заполненность буфера
            ctx->buffer_size = 0;
        }
    }
}

void stribog_final(stribog_ctx_t *ctx, uint8_t *hash) {
    /*
    Функция завершения хеширования. Вызывается в случае,
    когда данные для хеширования закончились, и необходимо получить значение хеша.
    Реализует 3 этап в алгоритме из ГОСТа.
    Возвращает хеш в порядке: в нулевой ячейке массива лежит нулевой (последний в нотации ГОСТа) байт хеша.
    */
    // Дополнение сообщения
    // Добавляем бит '1'
    ctx->buffer[ctx->buffer_size] = 0x01;
    ctx->buffer_size++;
    
    // Заполняем оставшуюся часть буфера нулями
    memset(ctx->buffer + ctx->buffer_size, 0, STRIBOG_BLOCK_SIZE - ctx->buffer_size);

    // Обрабатываем последнюю часть сообщения
    stribog_g_transform(ctx->N, ctx->h, ctx->buffer, ctx->h);
    // Прибавляем к N длину обработанного сообщения в битах
    stribog_add_number_modulo_512(ctx->N, (ctx->buffer_size - 1) * 8);
    // Прибавляем к Сигме последний блок сообщения, преобразованный в число
    stribog_add_array_modulo_512(ctx->Sigma, ctx->buffer);

    // Вызываем g с нулевым индексом и параметрами g0(h, N)
    uint8_t zero[STRIBOG_BLOCK_SIZE] = {0};
    stribog_g_transform(zero, ctx->h, ctx->N, ctx->h);
    // Вызываем g с нулевым индексом и параметрами g0(h, Sigma)
    stribog_g_transform(zero, ctx->h, ctx->Sigma, hash);
    
    // Для Stribog-256 берем только первые 256 бит (4 элемента из 8)
    if (ctx->hash_size == 256) {
        // Конвертируем результат в байты (little-endian)
        memcpy(hash, &hash[32], 32);
        memset(&hash[32], 0, 32);
    }
}
